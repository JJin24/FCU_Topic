#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define MAX_FLOWS 1024
#define PRE_SAVE_PACKET_COUNT 6 // n: 預設儲存前6個封包
#define TRAFFIC_MONITOR_INTERVAL 10 // T: 預設10秒監測流量
#define FLOW_TIMEOUT_SECONDS 60 // Flow 超時時間，可根據需要調整
#define SOCKET_PATH "/tmp/Flow2img_II_Proxy.sock"

// Flow 的資料結構
typedef struct {
    char name[256]; // 增加長度以包含時間戳
    pcap_dumper_t *pcap_dumper_full;
    pcap_dumper_t *pcap_dumper_pre;
    int packet_count;
    pthread_t thread_id;
    int save_files;
    time_t first_packet_time; // 首個封包的時間
    time_t last_packet_time;  // 最後一個封包的時間
    int is_active;            // 標記 Flow 是否活躍
    pcap_t *pcap_dead_handle; // 用於 pcap_dump_open 的 dead handle
} Flow;

// Flow2img_II_Proxy 結構，用於傳輸給 Socket
typedef struct {
    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;
    time_t first_packet_time;
    char pcap_filename[256]; // 儲存生成的 .pcap 檔名
} Flow2img_II_Proxy;

// 傳遞給子執行緒的參數
typedef struct {
    Flow *flow;
    const struct pcap_pkthdr *header;
    const u_char *packet;
    pcap_t *pcap_handle_for_dumper; // 傳遞給子執行緒用於 pcap_dump_open
} ThreadArgs;

Flow *flows[MAX_FLOWS];
pthread_mutex_t flows_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t traffic_mutex = PTHREAD_MUTEX_INITIALIZER;
unsigned long long total_bytes_sniffed = 0;

int enable_save_files_global = 0; // 全局變量控制是否儲存檔案
int select_interface = 1; // 是否選擇網卡，0 為監聽所有網卡

// 將數據轉換為大端序
uint16_t htons_wrapper(uint16_t hostshort) {
    return htons(hostshort);
}

uint32_t htonl_wrapper(uint32_t hostlong) {
    return htonl(hostlong);
}

// 傳送 Flow2img_II_Proxy 結構到 Socket 的函式
void send_flow_to_socket(const Flow2img_II_Proxy *flow_proxy) {
    int sockfd;
    struct sockaddr_un remote;
    size_t len;

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return;
    }

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, SOCKET_PATH);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);

    if (connect(sockfd, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        close(sockfd);
        return;
    }

    // 1. 傳送 sizeof(Flow2img_II_Proxy)
    size_t proxy_size = sizeof(Flow2img_II_Proxy);
    size_t proxy_size_be = htonl_wrapper(proxy_size); // 轉換為大端序
    if (send(sockfd, &proxy_size_be, sizeof(size_t), 0) == -1) {
        perror("send sizeof(Flow2img_II_Proxy)");
        close(sockfd);
        return;
    }

    // 2. 傳送 Flow2img_II_Proxy 結構
    Flow2img_II_Proxy flow_proxy_be = *flow_proxy;
    flow_proxy_be.src_port = htons_wrapper(flow_proxy_be.src_port);
    flow_proxy_be.dst_port = htons_wrapper(flow_proxy_be.dst_port);

    if (send(sockfd, &flow_proxy_be, sizeof(Flow2img_II_Proxy), 0) == -1) {
        perror("send Flow2img_II_Proxy");
        close(sockfd);
        return;
    }

    close(sockfd);
}

// 清理單個 Flow 的資源並傳送給 Socket
void cleanup_flow_and_send(Flow *flow, const char *src_ip, const char *dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t protocol) {
    if (flow->save_files) {
        if (flow->pcap_dumper_full != NULL) {
            pcap_dump_close(flow->pcap_dumper_full);
            flow->pcap_dumper_full = NULL;
        }
        if (flow->pcap_dumper_pre != NULL) {
            pcap_dump_close(flow->pcap_dumper_pre);
            flow->pcap_dumper_pre = NULL;
        }
        if (flow->pcap_dead_handle != NULL) {
            pcap_close(flow->pcap_dead_handle);
            flow->pcap_dead_handle = NULL;
        }

        // 準備 Flow2img_II_Proxy 結構
        Flow2img_II_Proxy proxy_data;
        strncpy(proxy_data.src_ip, src_ip, INET_ADDRSTRLEN);
        strncpy(proxy_data.dst_ip, dst_ip, INET_ADDRSTRLEN);
        proxy_data.src_port = src_port;
        proxy_data.dst_port = dst_port;
        proxy_data.protocol = protocol;
        proxy_data.first_packet_time = flow->first_packet_time;

        char full_filename[256];
        snprintf(full_filename, sizeof(full_filename), "%s_full.pcap", flow->name);
        strncpy(proxy_data.pcap_filename, full_filename, sizeof(proxy_data.pcap_filename));

        // 傳送資料給 Socket
        send_flow_to_socket(&proxy_data);
    }
}

// 子執行緒的處理函式
void *process_packet(void *args) {
    ThreadArgs *thread_args = (ThreadArgs *)args;
    Flow *flow = thread_args->flow;
    const struct pcap_pkthdr *header = thread_args->header;
    const u_char *packet = thread_args->packet;
    pcap_t *pcap_handle_for_dumper = thread_args->pcap_handle_for_dumper;

    pthread_mutex_lock(&flows_mutex);
    flow->last_packet_time = time(NULL); // 更新最後封包時間
    flow->is_active = 1; // 標記為活躍
    pthread_mutex_unlock(&flows_mutex);

    if (flow->save_files) {
        // 儲存前PRE_SAVE_PACKET_COUNT個封包
        if (flow->packet_count < PRE_SAVE_PACKET_COUNT) { // 修正為 <
            if (flow->pcap_dumper_pre == NULL) {
                char pre_filename[256];
                snprintf(pre_filename, sizeof(pre_filename), "%s_pre.pcap", flow->name);
                flow->pcap_dumper_pre = pcap_dump_open(pcap_handle_for_dumper, pre_filename);
            }
            if (flow->pcap_dumper_pre != NULL) {
                pcap_dump((u_char *)flow->pcap_dumper_pre, header, packet);
                pcap_dump_flush(flow->pcap_dumper_pre);
            }
        }

        // 儲存完整 Flow
        if (flow->pcap_dumper_full == NULL) {
            char full_filename[256];
            snprintf(full_filename, sizeof(full_filename), "%s_full.pcap", flow->name);
            flow->pcap_dumper_full = pcap_dump_open(pcap_handle_for_dumper, full_filename);
        }
        if (flow->pcap_dumper_full != NULL) {
            pcap_dump((u_char *)flow->pcap_dumper_full, header, packet);
            pcap_dump_flush(flow->pcap_dumper_full);
        }
    }

    flow->packet_count++;
    free(thread_args);
    return NULL;
}

// 封包處理的回呼函式
void packet_handler(u_char *user_data, const struct pcap_pkthdr *header, const u_char *packet) {
    // 增加監測流量
    pthread_mutex_lock(&traffic_mutex);
    total_bytes_sniffed += header->len;
    pthread_mutex_unlock(&traffic_mutex);

    // 乙太網頭部通常是14個字節
    if (header->len < 14 + sizeof(struct ip)) {
        // 封包太小，不是完整的IP封包
        return;
    }

    struct ip *ip_header = (struct ip *)(packet + 14);
    
    // 檢查 IP 版本，只處理 IPv4
    if (ip_header->ip_hl < 5) { // 確保 IP 頭部至少有 20 字節
        return;
    }

    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ip_header->ip_src), src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(ip_header->ip_dst), dst_ip, INET_ADDRSTRLEN);

    int src_port = 0;
    int dst_port = 0;
    
    // 確保封包足夠長以包含傳輸層頭部
    unsigned int ip_header_len = ip_header->ip_hl * 4;
    unsigned int min_packet_len = 14 + ip_header_len;

    if (header->len < min_packet_len) {
        return;
    }

    if (ip_header->ip_p == IPPROTO_TCP) {
        if (header->len < min_packet_len + sizeof(struct tcphdr)) return;
        struct tcphdr *tcp_header = (struct tcphdr *)(packet + 14 + ip_header_len);
        src_port = ntohs(tcp_header->th_sport);
        dst_port = ntohs(tcp_header->th_dport);
    } else if (ip_header->ip_p == IPPROTO_UDP) {
        if (header->len < min_packet_len + sizeof(struct udphdr)) return;
        struct udphdr *udp_header = (struct udphdr *)(packet + 14 + ip_header_len);
        src_port = ntohs(udp_header->uh_sport);
        dst_port = ntohs(udp_header->uh_dport);
    } else if (ip_header->ip_p == IPPROTO_ICMP) {
        // ICMP 不使用端口，port 保持為 0
    }
    // TODO: 可以添加其他協議的處理

    char flow_name_base[128];
    snprintf(flow_name_base, sizeof(flow_name_base), "%s_%d-%s_%d_%d", src_ip, src_port, dst_ip, dst_port, ip_header->ip_p);

    pthread_mutex_lock(&flows_mutex);
    int flow_index = -1;
    time_t current_time = time(NULL);

    // 尋找現有的 Flow
    for (int i = 0; i < MAX_FLOWS; i++) {
        if (flows[i] != NULL && flows[i]->is_active && strcmp(flows[i]->name, flow_name_base) == 0) { // 只匹配基本名稱
            flow_index = i;
            break;
        }
    }

    if (flow_index == -1) { // 如果沒有找到現有的 Flow，則建立新的 Flow
        for (int i = 0; i < MAX_FLOWS; i++) {
            if (flows[i] == NULL || !flows[i]->is_active) { // 尋找空的或非活躍的槽位
                flow_index = i;
                break;
            }
        }

        if (flow_index != -1) {
            // 初始化新的 Flow
            if (flows[flow_index] != NULL) {
                // 如果是重複利用非活躍的 Flow，則先清理舊資源
                // 這裡需要擷取舊 Flow 的五元組來清理
                char old_src_ip[INET_ADDRSTRLEN], old_dst_ip[INET_ADDRSTRLEN];
                uint16_t old_src_port, old_dst_port;
                uint8_t old_protocol;
                // 從 flow->name 解析舊的五元組，這裡省略複雜的解析，假設在 cleanup_flow_and_send 內部處理
                // 實際應用中，你可能需要將這些資訊儲存在 Flow 結構中

                // 為了簡化，這裡假設直接使用當前封包的五元組進行清理 (這可能不嚴謹，但為了範例)
                cleanup_flow_and_send(flows[flow_index], src_ip, dst_ip, src_port, dst_port, ip_header->ip_p);
                free(flows[flow_index]);
            }
            
            flows[flow_index] = (Flow *)malloc(sizeof(Flow));
            if (flows[flow_index] == NULL) {
                fprintf(stderr, "Failed to allocate memory for Flow\n");
                pthread_mutex_unlock(&flows_mutex);
                return;
            }

            // 包含首個封包時間的檔名
            char timestamp_str[32];
            strftime(timestamp_str, sizeof(timestamp_str), "%Y%m%d%H%M%S", localtime(&current_time));
            snprintf(flows[flow_index]->name, sizeof(flows[flow_index]->name), "%s_%s", flow_name_base, timestamp_str);
            
            flows[flow_index]->pcap_dumper_full = NULL;
            flows[flow_index]->pcap_dumper_pre = NULL;
            flows[flow_index]->packet_count = 0;
            flows[flow_index]->save_files = enable_save_files_global; // 使用全局變量
            flows[flow_index]->first_packet_time = current_time;
            flows[flow_index]->last_packet_time = current_time;
            flows[flow_index]->is_active = 1;
            flows[flow_index]->pcap_dead_handle = pcap_open_dead(DLT_EN10MB, 65535); // 每個 Flow 有自己的 dead handle
            if (flows[flow_index]->pcap_dead_handle == NULL) {
                 fprintf(stderr, "Failed to open dead handle for flow %s\n", flows[flow_index]->name);
                 free(flows[flow_index]);
                 flows[flow_index] = NULL;
                 pthread_mutex_unlock(&flows_mutex);
                 return;
            }
        }
    }
    pthread_mutex_unlock(&flows_mutex);

    if (flow_index != -1) {
        ThreadArgs *args = (ThreadArgs *)malloc(sizeof(ThreadArgs));
        if (args == NULL) {
            fprintf(stderr, "Failed to allocate memory for ThreadArgs\n");
            return;
        }
        args->flow = flows[flow_index];
        args->header = header;
        args->packet = packet;
        args->pcap_handle_for_dumper = flows[flow_index]->pcap_dead_handle; // 傳遞給子執行緒

        pthread_create(&flows[flow_index]->thread_id, NULL, process_packet, (void *)args);
        pthread_detach(flows[flow_index]->thread_id); // 分離子執行緒，使其在結束時自動釋放資源
    }
}

// 流量監測執行緒
void *traffic_monitor_thread(void *arg) {
    unsigned long long prev_total_bytes = 0;
    while (1) {
        sleep(TRAFFIC_MONITOR_INTERVAL);
        pthread_mutex_lock(&traffic_mutex);
        unsigned long long current_total_bytes = total_bytes_sniffed;
        pthread_mutex_unlock(&traffic_mutex);

        unsigned long long bytes_in_interval = current_total_bytes - prev_total_bytes;
        double traffic_mbps = (double)bytes_in_interval * 8 / (TRAFFIC_MONITOR_INTERVAL * 1000 * 1000); // 轉換為 Mbps
        printf("Traffic in last %d seconds: %.2f Mbps\n", TRAFFIC_MONITOR_INTERVAL, traffic_mbps);
        prev_total_bytes = current_total_bytes;
    }
    return NULL;
}

// Flow 超時檢查執行緒
void *flow_timeout_checker_thread(void *arg) {
    while (1) {
        sleep(1); // 每秒檢查一次
        time_t current_time = time(NULL);

        pthread_mutex_lock(&flows_mutex);
        for (int i = 0; i < MAX_FLOWS; i++) {
            if (flows[i] != NULL && flows[i]->is_active) {
                // 如果 Flow 超時
                if (current_time - flows[i]->last_packet_time > FLOW_TIMEOUT_SECONDS) {
                    printf("Flow %s timed out.\n", flows[i]->name);
                    
                    // 從 Flow 名稱解析五元組
                    char src_ip_str[INET_ADDRSTRLEN];
                    char dst_ip_str[INET_ADDRSTRLEN];
                    uint16_t src_p = 0;
                    uint16_t dst_p = 0;
                    uint8_t proto = 0;
                    
                    // 解析 flow->name (例如: 192.168.1.1_12345-192.168.1.2_80_6_YYYYMMDDHHMMSS)
                    // 這裡需要一個更健壯的解析函式，為了簡潔，這裡做一個簡單的假設
                    // 實際應用中，考慮將這些資訊直接儲存在 Flow 結構中
                    char name_copy[256];
                    strncpy(name_copy, flows[i]->name, sizeof(name_copy));
                    char *token;
                    char *rest = name_copy;

                    // src_ip
                    token = strtok_r(rest, "_", &rest);
                    if (token) strncpy(src_ip_str, token, sizeof(src_ip_str));
                    
                    // src_port
                    token = strtok_r(rest, "-", &rest);
                    if (token) src_p = (uint16_t)atoi(token); // 需要跳過下劃線後的埠號
                    
                    // dst_ip
                    token = strtok_r(rest, "_", &rest);
                    if (token) strncpy(dst_ip_str, token, sizeof(dst_ip_str));

                    // dst_port
                    token = strtok_r(rest, "_", &rest);
                    if (token) dst_p = (uint16_t)atoi(token);

                    // protocol
                    token = strtok_r(rest, "_", &rest);
                    if (token) proto = (uint8_t)atoi(token);

                    cleanup_flow_and_send(flows[i], src_ip_str, dst_ip_str, src_p, dst_p, proto);
                    free(flows[i]);
                    flows[i] = NULL;
                }
            }
        }
        pthread_mutex_unlock(&flows_mutex);
    }
    return NULL;
}

// Socket 服務器初始化 (用於偵聽連接)
void *socket_server_init_thread(void *arg) {
    int listen_sockfd, client_sockfd;
    struct sockaddr_un local, remote;
    socklen_t len;

    unlink(SOCKET_PATH); // 確保之前沒有殘留的 socket 檔案

    if ((listen_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket server");
        return NULL;
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, SOCKET_PATH);
    len = strlen(local.sun_path) + sizeof(local.sun_family);

    if (bind(listen_sockfd, (struct sockaddr *)&local, len) == -1) {
        perror("bind server");
        close(listen_sockfd);
        return NULL;
    }

    if (listen(listen_sockfd, 5) == -1) {
        perror("listen server");
        close(listen_sockfd);
        return NULL;
    }

    printf("Socket server listening on %s\n", SOCKET_PATH);

    while (1) {
        len = sizeof(remote);
        if ((client_sockfd = accept(listen_sockfd, (struct sockaddr *)&remote, &len)) == -1) {
            perror("accept server");
            continue;
        }

        // 這裡可以處理來自客戶端的連接 (如果需要接收數據，目前只是傳送)
        // 為了確保 Flow2img_II_Proxy 能夠傳送，此服務器端只是保持開放
        // 如果 Proxy 程式會主動連接，這裡可以省略具體的接收邏輯
        close(client_sockfd); // 處理完畢後關閉客戶端連接
    }
    close(listen_sockfd);
    return NULL;
}


int main() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevs, *dev;

    // 啟動 Socket 服務器執行緒
    pthread_t socket_server_tid;
    pthread_create(&socket_server_tid, NULL, socket_server_init_thread, NULL);
    pthread_detach(socket_server_tid); // 分離執行緒

    // 啟動流量監測執行緒
    pthread_t traffic_tid;
    pthread_create(&traffic_tid, NULL, traffic_monitor_thread, NULL);
    pthread_detach(traffic_tid);

    // 啟動 Flow 超時檢查執行緒
    pthread_t flow_timeout_tid;
    pthread_create(&flow_timeout_tid, NULL, flow_timeout_checker_thread, NULL);
    pthread_detach(flow_timeout_tid);

    
    // printf("Do you want to select a specific network interface? (1 for yes, 0 for no): ");
    // scanf("%d", &select_interface);

    pcap_t *handle = NULL;
    if (select_interface) {
        if (pcap_findalldevs(&alldevs, errbuf) == -1) {
            fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
            return 1;
        }

        int i = 0;
        for (dev = alldevs; dev; dev = dev->next) {
            printf("%d. %s\n", ++i, dev->name);
        }

        if (i == 0) {
            fprintf(stderr, "No network devices found.\n");
            pcap_freealldevs(alldevs);
            return 1;
        }

        printf("Enter the interface number (1-%d): ", i);
        int dev_num;
        scanf("%d", &dev_num);

        if (dev_num < 1 || dev_num > i) {
            fprintf(stderr, "Invalid interface number.\n");
            pcap_freealldevs(alldevs);
            return 1;
        }

        for (dev = alldevs, i = 0; i < dev_num - 1; dev = dev->next, i++);

        printf("Enable promiscuous mode? (1 for yes, 0 for no): ");
        int promisc_mode;
        scanf("%d", &promisc_mode);

        // printf("Save captured files? (1 for yes, 0 for no): ");
        // scanf("%d", &enable_save_files_global); // 設定全局變量

        handle = pcap_open_live(dev->name, BUFSIZ, promisc_mode, 1000, errbuf);
        if (handle == NULL) {
            fprintf(stderr, "Couldn't open device %s: %s\n", dev->name, errbuf);
            pcap_freealldevs(alldevs);
            return 2;
        }
        printf("Start sniffing on interface %s...\n", dev->name);

    } else { // 監聽所有網卡
        // printf("Save captured files? (1 for yes, 0 for no): ");
        // scanf("%d", &enable_save_files_global); // 設定全局變量

        if (pcap_findalldevs(&alldevs, errbuf) == -1) {
            fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
            return 1;
        }

        pcap_if_t *current_dev;
        for (current_dev = alldevs; current_dev; current_dev = current_dev->next) {
            pcap_t *temp_handle = pcap_open_live(current_dev->name, BUFSIZ, 1, 1000, errbuf); // promiscuous mode 預設為 1
            if (temp_handle == NULL) {
                fprintf(stderr, "Couldn't open device %s: %s\n", current_dev->name, errbuf);
                continue;
            }
            printf("Sniffing on all interfaces... (Currently on %s)\n", current_dev->name);
            pcap_loop(temp_handle, -1, packet_handler, (u_char *)&enable_save_files_global);
            pcap_close(temp_handle); // 每個介面處理完畢後關閉
        }
        pcap_freealldevs(alldevs);
        return 0; // 所有介面都監聽完畢，程式結束
    }

    pcap_loop(handle, -1, packet_handler, (u_char *)&enable_save_files_global);

    pcap_close(handle);
    pcap_freealldevs(alldevs);

    // 清理資源 (在 main 結束時，這裡需要確保所有 Flow 都已清理)
    // 實際上，由於 detach 子執行緒和 timeout 機制，大部分清理會在執行緒中完成
    // 這裡主要處理那些在 main 退出時仍然活躍的 Flow
    for (int j = 0; j < MAX_FLOWS; j++) {
        if (flows[j] != NULL) {
            // 從 Flow 名稱解析五元組 (同樣需要健壯的解析)
            char src_ip_str[INET_ADDRSTRLEN];
            char dst_ip_str[INET_ADDRSTRLEN];
            uint16_t src_p = 0;
            uint16_t dst_p = 0;
            uint8_t proto = 0;

            // 這裡為了簡潔，假設可以直接從 flows[j]->name 中解析，實際應用需要仔細實現
            // 例如：sscanf(flows[j]->name, "%[^_]_%hu-%[^_]_%hu_%hhu", src_ip_str, &src_p, dst_ip_str, &dst_p, &proto);

            cleanup_flow_and_send(flows[j], src_ip_str, dst_ip_str, src_p, dst_p, proto);
            free(flows[j]);
            flows[j] = NULL;
        }
    }

    // 移除 Socket 檔案
    unlink(SOCKET_PATH);

    return 0;
}