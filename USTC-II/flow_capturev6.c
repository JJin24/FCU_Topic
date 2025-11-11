/* ===== 程式總覽（逐階段說明） =====
    階段 A: 初始包含與常數定義（包含 pcap 相關與 thread mutex）
    階段 B: Flow 資料結構定義與全域變數
    階段 C: 工具函式（htons_wrapper / htonl_wrapper）
    階段 D: 與外部 socket 溝通的函式（send_flow_to_socket）
    階段 E: pcap 檔案的傳送與刪除（send_and_delete_pcap / cleanup_flow_and_send）
    階段 F: 每個封包的處理子執行緒（process_packet）
    階段 G: pcap_loop 的封包回呼（packet_handler）
    階段 H: 背景執行緒（traffic monitor / flow timeout checker）
    階段 I: 主流程（main - 介面選擇、開啟 pcap、啟動回圈）
*/
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
#include <curl/curl.h>
#include "common.h"

#define MAX_FLOWS 1024

#define TRAFFIC_MONITOR_INTERVAL 10 // T: 預設10秒監測流量
#define FLOW_TIMEOUT_SECONDS 30 // Flow 超時時間
// --- 從 common.h 引入的結構定義 ---
#define MAX_PCAP_DATA_SIZE (MAX_PKT_LEN * N_PKTS)

const char *url = "http://127.0.0.1:3000/traffic/report";

// 測試控制元
int promisc_mode = 1;  //混淆模式，預設開啟

// Flow 的資料結構
typedef struct {
    char name[240];
    pcap_dumper_t *pcap_dumper_pre;
    int packet_count;
    pthread_t thread_id;
    time_t first_packet_time;
    time_t last_packet_time;
    int is_active;
    int pcap_sent; // pcap 檔案是否已傳送
    pcap_t *pcap_dead_handle;
} Flow;

// 傳遞給子執行緒的參數
typedef struct {
    Flow *flow;
    const struct pcap_pkthdr *header;
    const u_char *packet;
    pcap_t *pcap_handle_for_dumper;
} ThreadArgs;

Flow *flows[MAX_FLOWS];
pthread_mutex_t flows_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t traffic_mutex = PTHREAD_MUTEX_INITIALIZER;
unsigned long long total_bytes_sniffed = 0;

// forward declaration for send_traffic_to_api (defined below)
void send_traffic_to_api(unsigned long long bytes, int interval_seconds);  //傳送為單位為MB/s


// 將數據轉換為大端序 (網路位元組序)
uint16_t htons_wrapper(uint16_t hostshort) {
    return htons(hostshort);
}

uint32_t htonl_wrapper(uint32_t hostlong) {
    return htonl(hostlong);
}

// 傳送 Flow2img_II_context 結構到 Socket 的函式
void send_flow_to_socket(const Flow2img_II_context *flow_context) {
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
        // perror("connect");
        close(sockfd);
        return;
    }

    // 1. 傳送 body_size (Flow2img_II_context 的大小)
    size_t context_size = sizeof(Flow2img_II_context);
    uint32_t context_size_be = htonl_wrapper(context_size); // 轉換為大端序
    if (send(sockfd, &context_size_be, sizeof(uint32_t), 0) == -1) {
        perror("send sizeof(Flow2img_II_context)");
        close(sockfd);
        return;
    }

    // 2. 準備並傳送 Flow2img_II_context 結構
    Flow2img_II_context context_be = *flow_context;
    context_be.pcap_size = htonl_wrapper(context_be.pcap_size);
    context_be.s_port = htons_wrapper(context_be.s_port);
    context_be.d_port = htons_wrapper(context_be.d_port);

    if (send(sockfd, &context_be, sizeof(Flow2img_II_context), 0) == -1) {
        perror("send Flow2img_II_context");
        close(sockfd);
        return;
    }

    printf("Successfully sent flow context for %s:%d -> %s:%d\n", context_be.s_ip, ntohs(context_be.s_port), context_be.d_ip, ntohs(context_be.d_port));
    close(sockfd);
}

// *** 修改 ***: 函式更名並調整職責，僅處理 pcap 檔案的傳送與刪除
void send_and_delete_pcap(Flow *flow, const char *src_ip, const char *dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t protocol, const char *reason) {
        char pre_filename[256];
        snprintf(pre_filename, sizeof(pre_filename), "%s_pre.pcap", flow->name);
        
        // 關閉 dumper 以確保檔案內容寫入完畢
        if (flow->pcap_dumper_pre != NULL) {
            pcap_dump_close(flow->pcap_dumper_pre);
            flow->pcap_dumper_pre = NULL;
        }

        // 這裡不再關閉 pcap_dead_handle，因為 Flow 還要繼續存在

        Flow2img_II_context context_data;
        memset(&context_data, 0, sizeof(Flow2img_II_context));

        snprintf(context_data.s_ip, sizeof(context_data.s_ip), "%s", src_ip);
        snprintf(context_data.d_ip, sizeof(context_data.d_ip), "%s", dst_ip);
        context_data.s_port = src_port;
        context_data.d_port = dst_port;
        context_data.protocol = protocol;
        
        strftime(context_data.timestamp, sizeof(context_data.timestamp), "%Y-%m-%d %H:%M:%S", localtime(&flow->first_packet_time));
        
        FILE *pcap_file = fopen(pre_filename, "rb");
        if (pcap_file) {
            fseek(pcap_file, 0, SEEK_END);
            long file_size = ftell(pcap_file);
            fseek(pcap_file, 0, SEEK_SET);

            if (file_size > 0 && file_size <= MAX_PCAP_DATA_SIZE) {
                size_t bytes_read = fread(context_data.pcap_data, 1, file_size, pcap_file);
                context_data.pcap_size = bytes_read;
                // print sending reason immediately before sending
                printf("Sending flow %s (%s): %s:%u -> %s:%u, pcap_size=%zu bytes\n",
                       flow->name, reason, context_data.s_ip, (unsigned)context_data.s_port,
                       context_data.d_ip, (unsigned)context_data.d_port, bytes_read);
                send_flow_to_socket(&context_data);
            }
            fclose(pcap_file);
        }
        remove(pre_filename);
}

// 子執行緒的處理函式
void *process_packet(void *args) {
    ThreadArgs *thread_args = (ThreadArgs *)args;
    Flow *flow = thread_args->flow;
    const struct pcap_pkthdr *header = thread_args->header;
    const u_char *packet = thread_args->packet;
    pcap_t *pcap_handle_for_dumper = thread_args->pcap_handle_for_dumper;

    int should_send = 0;

    pthread_mutex_lock(&flows_mutex);
    flow->last_packet_time = time(NULL);
    flow->is_active = 1;

    // *** 修改 ***: 只有在 packet_count < N_PKTS 時才寫入檔案
    if (flow->packet_count < N_PKTS) {
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
    
    flow->packet_count++;

    // *** 修改 ***: 檢查是否剛好達到 N_PKTS 且尚未傳送過
    if (flow->packet_count == N_PKTS && !flow->pcap_sent) {
        should_send = 1;
        flow->pcap_sent = 1; // 標記為已傳送，防止其他執行緒重複操作
    }
    pthread_mutex_unlock(&flows_mutex);

    // 如果達到 N_PKTS，則呼叫函式傳送檔案
    if (should_send) {
        char src_ip_str[INET_ADDRSTRLEN];
        char dst_ip_str[INET_ADDRSTRLEN];
        uint16_t src_p = 0;
        uint16_t dst_p = 0;
        uint8_t proto = 0;
        sscanf(flow->name, "%[^_]_%hu-%[^_]_%hu_%hhu",
               src_ip_str, &src_p, dst_ip_str, &dst_p, &proto);

        // 呼叫新的函式，只傳送檔案，不清理 Flow，標註為 N_PKTS 觸發
        send_and_delete_pcap(flow, src_ip_str, dst_ip_str, src_p, dst_p, proto, "N_PKTS");
        // 此處不再修改 is_active 或釋放 flow，讓它繼續存在
    }

    free(thread_args);
    return NULL;
}


// 封包處理的回呼函式
void packet_handler(u_char *user_data, const struct pcap_pkthdr *header, const u_char *packet) {
    (void)user_data;

    pthread_mutex_lock(&traffic_mutex);
    total_bytes_sniffed += header->len;
    pthread_mutex_unlock(&traffic_mutex);

    if (header->len < 14 + sizeof(struct ip)) {
        return;
    }

    struct ip *ip_header = (struct ip *)(packet + 14);
    
    if (ip_header->ip_v != 4 || ip_header->ip_hl < 5) {
        return;
    }

    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ip_header->ip_src), src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(ip_header->ip_dst), dst_ip, INET_ADDRSTRLEN);

    uint16_t src_port = 0;
    uint16_t dst_port = 0;
    
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
    }

    // Use the first-seen packet direction as the canonical direction for a new flow.
    // To find existing flows regardless of packet direction, we'll build both
    // forward and reverse flow name bases and match either one.
    char flow_name_base[128];
    char flow_name_base_rev[128];
    snprintf(flow_name_base, sizeof(flow_name_base), "%s_%d-%s_%d_%d", src_ip, src_port, dst_ip, dst_port, ip_header->ip_p);
    snprintf(flow_name_base_rev, sizeof(flow_name_base_rev), "%s_%d-%s_%d_%d", dst_ip, dst_port, src_ip, src_port, ip_header->ip_p);

    pthread_mutex_lock(&flows_mutex);
    int flow_index = -1;
    time_t current_time = time(NULL);

    for (int i = 0; i < MAX_FLOWS; i++) {
        if (flows[i] != NULL && flows[i]->is_active) {
            if (strncmp(flows[i]->name, flow_name_base, strlen(flow_name_base)) == 0 ||
                strncmp(flows[i]->name, flow_name_base_rev, strlen(flow_name_base_rev)) == 0) {
                flow_index = i;
                break;
            }
        }
    }
    // flow[i]->name 皆未匹配到
    if (flow_index == -1) {
        for (int i = 0; i < MAX_FLOWS; i++) {
            if (flows[i] == NULL) {
                flow_index = i;
                break;
            }
        }
        // flow[] 還有位置
        if (flow_index != -1) {
            flows[flow_index] = (Flow *)malloc(sizeof(Flow));
            if (flows[flow_index] == NULL) {
                fprintf(stderr, "Failed to allocate memory for Flow\n");
                pthread_mutex_unlock(&flows_mutex);
                return;
            }

            char timestamp_str[32];
            strftime(timestamp_str, sizeof(timestamp_str), "%Y%m%d%H%M%S", localtime(&current_time));
            snprintf(flows[flow_index]->name, sizeof(flows[flow_index]->name), "%s_%s", flow_name_base, timestamp_str);
            
            flows[flow_index]->pcap_dumper_pre = NULL;
            flows[flow_index]->packet_count = 0;
            flows[flow_index]->first_packet_time = current_time;
            flows[flow_index]->last_packet_time = current_time;
            flows[flow_index]->is_active = 1;
            flows[flow_index]->pcap_sent = 0; 
            flows[flow_index]->pcap_dead_handle = pcap_open_dead(DLT_EN10MB, 65535);
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
        args->pcap_handle_for_dumper = flows[flow_index]->pcap_dead_handle;

        pthread_create(&flows[flow_index]->thread_id, NULL, process_packet, (void *)args);
        pthread_detach(flows[flow_index]->thread_id);
    }
}

// 流量監測執行緒
void *traffic_monitor_thread(void *arg) {
    (void)arg;

    unsigned long long prev_total_bytes = 0;
    while (1) {
        sleep(TRAFFIC_MONITOR_INTERVAL);
        pthread_mutex_lock(&traffic_mutex);
        unsigned long long current_total_bytes = total_bytes_sniffed;
        pthread_mutex_unlock(&traffic_mutex);

        unsigned long long bytes_in_interval = current_total_bytes - prev_total_bytes;
        
        double traffic_mbps = (double)bytes_in_interval * 8 / (1000 * 1000);
        printf("Traffic in last %d seconds: %.2f MB\n", TRAFFIC_MONITOR_INTERVAL, traffic_mbps);
        // send the aggregate bytes to API
        // best-effort: ignore failures
        send_traffic_to_api(bytes_in_interval, TRAFFIC_MONITOR_INTERVAL);
        prev_total_bytes = current_total_bytes;
    }
    return NULL;
}

// Send traffic report to local API endpoint using libcurl
void send_traffic_to_api(unsigned long long bytes, int interval_seconds) { 
    CURL *curl = curl_easy_init();
    if (!curl) return;

    //const char *url = "http://127.0.0.1:3000/traffic/report";
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    char postdata[256];
    snprintf(postdata, sizeof(postdata), "{\"interval_seconds\": %d, \"bytes\": %llu}", interval_seconds, bytes);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 2000L); // 2s timeout

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Failed to POST traffic report: %s\n", curl_easy_strerror(res));
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

// Flow 超時檢查執行緒
void *flow_timeout_checker_thread(void *arg) {
    (void)arg;

    while (1) {
        sleep(1);
        time_t current_time = time(NULL);

        pthread_mutex_lock(&flows_mutex);
        for (int i = 0; i < MAX_FLOWS; i++) {
            if (flows[i] != NULL && flows[i]->is_active) {
                if (current_time - flows[i]->last_packet_time > FLOW_TIMEOUT_SECONDS) {
                    printf("Flow %s timed out. Cleaning up.\n", flows[i]->name);
                    
                    //只有在 pcap 尚未傳送時才進行傳送
                    if (!flows[i]->pcap_sent) {
                        char src_ip_str[INET_ADDRSTRLEN];
                        char dst_ip_str[INET_ADDRSTRLEN];
                        uint16_t src_p = 0;
                        uint16_t dst_p = 0;
                        uint8_t proto = 0;
                        
                        sscanf(flows[i]->name, "%[^_]_%hu-%[^_]_%hu_%hhu", 
                               src_ip_str, &src_p, dst_ip_str, &dst_p, &proto);

                        // 超時前未達到 N_PKTS，傳送現有的封包（標註為 TIMEOUT）
                        send_and_delete_pcap(flows[i], src_ip_str, dst_ip_str, src_p, dst_p, proto, "TIMEOUT");
                    }

                    // *** 修改 ***: 執行最終的清理
                    if (flows[i]->pcap_dead_handle != NULL) {
                        pcap_close(flows[i]->pcap_dead_handle);
                    }
                    free(flows[i]);
                    flows[i] = NULL;
                }
            }
        }
        pthread_mutex_unlock(&flows_mutex);
    }
    return NULL;
}

int main() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevs, *dev;

    pcap_t *handle = NULL;
    // initialize libcurl global state for HTTP POST used in traffic monitor
    curl_global_init(CURL_GLOBAL_ALL);
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
        
        if (scanf("%d", &dev_num) != 1) {
            fprintf(stderr, "Invalid input. Please enter a number.\n");
            pcap_freealldevs(alldevs);
            return 1;
        }

         pthread_t traffic_tid, flow_timeout_tid;
         pthread_create(&traffic_tid, NULL, traffic_monitor_thread, NULL);
         pthread_detach(traffic_tid);
         pthread_create(&flow_timeout_tid, NULL, flow_timeout_checker_thread, NULL);
         pthread_detach(flow_timeout_tid);

        if (dev_num < 1 || dev_num > i) {
            fprintf(stderr, "Invalid interface number.\n");
            pcap_freealldevs(alldevs);
            return 1;
        }

        for (dev = alldevs, i = 0; i < dev_num - 1; dev = dev->next, i++);


        if (promisc_mode == 0){
           printf("Enable promiscuous mode? (1 for yes, 0 for no): ");
        
            if (scanf("%d", &promisc_mode) != 1) {
                fprintf(stderr, "Invalid input. Please enter a number.\n");
                pcap_freealldevs(alldevs);
                return 1;
            }
    }

        handle = pcap_open_live(dev->name, BUFSIZ, promisc_mode, 1000, errbuf);
        if (handle == NULL) {
            fprintf(stderr, "Couldn't open device %s: %s\n", dev->name, errbuf);
            pcap_freealldevs(alldevs);
            return 2;
        }
        printf("Start sniffing on interface %s...\n", dev->name);

    pcap_loop(handle, -1, packet_handler, NULL);

    pcap_close(handle);
    if (alldevs) {
        pcap_freealldevs(alldevs);
    }
    
    // 程式結束前清空未處理的Flow及pcap
    for (int j = 0; j < MAX_FLOWS; j++) {
        if (flows[j] != NULL) {
            if (!flows[j]->pcap_sent) {
                 char src_ip_str[INET_ADDRSTRLEN];
                 char dst_ip_str[INET_ADDRSTRLEN];
                 uint16_t src_p = 0;
                 uint16_t dst_p = 0;
                 uint8_t proto = 0;
            
                 sscanf(flows[j]->name, "%[^_]_%hu-%[^_]_%hu_%hhu", 
                        src_ip_str, &src_p, dst_ip_str, &dst_p, &proto);

                 send_and_delete_pcap(flows[j], src_ip_str, dst_ip_str, src_p, dst_p, proto, "EXIT");
            }
            if (flows[j]->pcap_dead_handle != NULL) {
                 pcap_close(flows[j]->pcap_dead_handle);
            }
            free(flows[j]);
            flows[j] = NULL;
        }
    }

    // cleanup libcurl
    curl_global_cleanup();

    return 0;
}