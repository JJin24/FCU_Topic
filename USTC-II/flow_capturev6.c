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

#include "common.h"

#define MAX_FLOWS 1024
#define TRAFFIC_MONITOR_INTERVAL 10 // T: 預設10秒監測流量
#define FLOW_TIMEOUT_SECONDS 60 // Flow 超時時間


// --- 從 common.h 引入的結構定義 ---
#define MAX_PCAP_DATA_SIZE (MAX_PKT_LEN * N_PKTS)

// 測試控制元
int enable_save_files_global = 1 ; // 預設啟用儲存檔案，因為需要讀取檔案內容
int select_interface = 1; 
int promisc_mode = 1;  //混淆模式，預設開啟
int del_pcap = 0 ;
/*
typedef struct {
    uint8_t pcap_data[MAX_PCAP_DATA_SIZE];
    uint32_t pcap_size;
    char s_ip[INET_ADDRSTRLEN];
    char d_ip[INET_ADDRSTRLEN];
    int s_port;
    int d_port;
    char timestamp[64];
    uint8_t protocol;
} Flow2img_II_context;
// --- 結束引入 ---
*/
// Flow 的資料結構
typedef struct {
    char name[240]; 
    pcap_dumper_t *pcap_dumper_pre;
    int packet_count;
    pthread_t thread_id;
    int save_files;
    time_t first_packet_time;
    time_t last_packet_time;
    int is_active;
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

// 清理單個 Flow 的資源，讀取 pcap 檔案內容並傳送給 Socket
void cleanup_flow_and_send(Flow *flow, const char *src_ip, const char *dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t protocol) {
    if (flow->save_files) {
        char pre_filename[256];
        snprintf(pre_filename, sizeof(pre_filename), "%s_pre.pcap", flow->name);
        if (flow->pcap_dumper_pre != NULL) {
            pcap_dump_close(flow->pcap_dumper_pre);
            flow->pcap_dumper_pre = NULL;
        }
        if (flow->pcap_dead_handle != NULL) {
            pcap_close(flow->pcap_dead_handle);
            flow->pcap_dead_handle = NULL;
        }

        Flow2img_II_context context_data;
        memset(&context_data, 0, sizeof(Flow2img_II_context));

        // 使用 snprintf 替代 strncpy 以確保安全 ---
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
                send_flow_to_socket(&context_data);
            }
            fclose(pcap_file);
        }
        remove(pre_filename);
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
    flow->last_packet_time = time(NULL);
    flow->is_active = 1;
    pthread_mutex_unlock(&flows_mutex);

    if (flow->save_files) {
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
    }

    flow->packet_count++;
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

    char flow_name_base[128];
    if (strcmp(src_ip, dst_ip) < 0 || (strcmp(src_ip, dst_ip) == 0 && src_port < dst_port)) {
        snprintf(flow_name_base, sizeof(flow_name_base), "%s_%d-%s_%d_%d", src_ip, src_port, dst_ip, dst_port, ip_header->ip_p);
    } else {
        snprintf(flow_name_base, sizeof(flow_name_base), "%s_%d-%s_%d_%d", dst_ip, dst_port, src_ip, src_port, ip_header->ip_p);
    }

    pthread_mutex_lock(&flows_mutex);
    int flow_index = -1;
    time_t current_time = time(NULL);

    for (int i = 0; i < MAX_FLOWS; i++) {
        if (flows[i] != NULL && flows[i]->is_active && strncmp(flows[i]->name, flow_name_base, strlen(flow_name_base)) == 0) {
            flow_index = i;
            break;
        }
    }

    if (flow_index == -1) {
        for (int i = 0; i < MAX_FLOWS; i++) {
            if (flows[i] == NULL) {
                flow_index = i;
                break;
            }
        }

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
            flows[flow_index]->save_files = enable_save_files_global;
            flows[flow_index]->first_packet_time = current_time;
            flows[flow_index]->last_packet_time = current_time;
            flows[flow_index]->is_active = 1;
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
        /*
        double traffic_mbps = (double)bytes_in_interval * 8 / (TRAFFIC_MONITOR_INTERVAL * 1000 * 1000);
        printf("Traffic in last %d seconds: %.2f Mbps\n", TRAFFIC_MONITOR_INTERVAL, traffic_mbps);
        */

        double traffic_mbps = (double)bytes_in_interval * 8 / (1000 * 1000);
        printf("Traffic in last %d seconds: %.2f MB\n", TRAFFIC_MONITOR_INTERVAL, traffic_mbps);
        prev_total_bytes = current_total_bytes;
    }
    return NULL;
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
                    
                    char src_ip_str[INET_ADDRSTRLEN];
                    char dst_ip_str[INET_ADDRSTRLEN];
                    uint16_t src_p = 0;
                    uint16_t dst_p = 0;
                    uint8_t proto = 0;
                    
                    sscanf(flows[i]->name, "%[^_]_%hu-%[^_]_%hu_%hhu", 
                           src_ip_str, &src_p, dst_ip_str, &dst_p, &proto);

                    flows[i]->is_active = 0; 
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

int main() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevs, *dev;

    pthread_t traffic_tid, flow_timeout_tid;
    pthread_create(&traffic_tid, NULL, traffic_monitor_thread, NULL);
    pthread_detach(traffic_tid);
    pthread_create(&flow_timeout_tid, NULL, flow_timeout_checker_thread, NULL);
    pthread_detach(flow_timeout_tid);

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
        
        // 檢查 scanf 的回傳值，確保成功讀取一個整數
        if (scanf("%d", &dev_num) != 1) {
            fprintf(stderr, "Invalid input. Please enter a number.\n");
            pcap_freealldevs(alldevs);
            return 1;
        }

        if (dev_num < 1 || dev_num > i) {
            fprintf(stderr, "Invalid interface number.\n");
            pcap_freealldevs(alldevs);
            return 1;
        }

        for (dev = alldevs, i = 0; i < dev_num - 1; dev = dev->next, i++);


        if (promisc_mode == 0){
           printf("Enable promiscuous mode? (1 for yes, 0 for no): ");
        

            // 同樣檢查 scanf 的回傳值
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

    } else {
        fprintf(stderr, "Listening on all interfaces is not fully supported in this simplified model.\n");
        return 1;
    }

    pcap_loop(handle, -1, packet_handler, NULL);

    pcap_close(handle);
    if (alldevs) {
        pcap_freealldevs(alldevs);
    }
    
    for (int j = 0; j < MAX_FLOWS; j++) {
        if (flows[j] != NULL) {
            char src_ip_str[INET_ADDRSTRLEN];
            char dst_ip_str[INET_ADDRSTRLEN];
            uint16_t src_p = 0;
            uint16_t dst_p = 0;
            uint8_t proto = 0;
            
            sscanf(flows[j]->name, "%[^_]_%hu-%[^_]_%hu_%hhu", 
                   src_ip_str, &src_p, dst_ip_str, &dst_p, &proto);

            cleanup_flow_and_send(flows[j], src_ip_str, dst_ip_str, src_p, dst_p, proto);
            free(flows[j]);
            flows[j] = NULL;
        }
    }

    return 0;
}