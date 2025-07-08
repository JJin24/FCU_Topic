#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/time.h>
#include <unistd.h>

#include "flow_tracker.h"

// 確保我們有正確的結構體定義
#ifndef TCPHDR_DEFINED
#define TCPHDR_DEFINED
struct my_tcphdr {
    uint16_t th_sport;  // source port
    uint16_t th_dport;  // destination port
    uint32_t th_seq;    // sequence number
    uint32_t th_ack;    // acknowledgement number
    uint8_t th_x2:4;    // (unused)
    uint8_t th_off:4;   // data offset
    uint8_t th_flags;
    uint16_t th_win;    // window
    uint16_t th_sum;    // checksum
    uint16_t th_urp;    // urgent pointer
};

struct my_udphdr {
    uint16_t uh_sport;  // source port
    uint16_t uh_dport;  // destination port
    uint16_t uh_ulen;   // udp length
    uint16_t uh_sum;    // udp checksum
};

struct my_iphdr {
    uint8_t ihl:4;      // Internet Header Length
    uint8_t version:4;  // Version
    uint8_t tos;        // Type of Service
    uint16_t tot_len;   // Total Length
    uint16_t id;        // Identification
    uint16_t frag_off;  // Fragment Offset
    uint8_t ttl;        // Time to Live
    uint8_t protocol;   // Protocol
    uint16_t check;     // Header Checksum
    uint32_t saddr;     // Source Address
    uint32_t daddr;     // Destination Address
};
#endif

// 全域變數，指向 Hash Table 的頭部
Flow *g_flows = NULL;

// 將 FlowKey 正規化，確保 A->B 和 B->A 的流量使用同一個 Key
static void normalize_flow_key(FlowKey *key) {
    if (key->ip_src > key->ip_dst) {
        uint32_t temp_ip = key->ip_src;
        key->ip_src = key->ip_dst;
        key->ip_dst = temp_ip;
        uint16_t temp_port = key->port_src;
        key->port_src = key->port_dst;
        key->port_dst = temp_port;
    } else if (key->ip_src == key->ip_dst && key->port_src > key->port_dst) {
        uint16_t temp_port = key->port_src;
        key->port_src = key->port_dst;
        key->port_dst = temp_port;
    }
}

// 根據 FlowKey 產生一個可讀的檔名
static void generate_pcap_filename(const FlowKey *key, char *filename_buf, size_t buf_len) {
    struct in_addr src, dst;
    src.s_addr = key->ip_src;
    dst.s_addr = key->ip_dst;
    snprintf(filename_buf, buf_len, "flow-%s_%u-%s_%u-proto%u.pcap",
             inet_ntoa(src), ntohs(key->port_src),
             inet_ntoa(dst), ntohs(key->port_dst),
             key->protocol);
}

// 實現函式：安全地關閉並釋放一個 Flow
void close_and_free_flow(Flow *flow, Flow **flows_hash) {
    if (!flow) return;
    printf("Closing flow: %s (%d packets)\n", flow->filename, flow->packet_count);
    pcap_dump_close(flow->pcap_dumper); // 關閉 pcap dumper
    HASH_DEL(*flows_hash, flow);       // 從 Hash Table 中移除
    free(flow);                        // 釋放記憶體
}

// 實現函式：檢查並移除超時的 Flow
void check_flow_timeouts(Flow **flows_hash) {
    Flow *current_flow, *tmp;
    struct timeval now;
    gettimeofday(&now, NULL);

    HASH_ITER(hh, *flows_hash, current_flow, tmp) {
        long diff_sec = now.tv_sec - current_flow->last_seen.tv_sec;
        if (diff_sec > FLOW_TIMEOUT_SECONDS) {
            printf("Flow timed out: %s\n", current_flow->filename);
            close_and_free_flow(current_flow, flows_hash);
        }
    }
}

// 實現函式：處理單一封包的 callback
void process_packet(uint8_t *args, const struct pcap_pkthdr *header, const uint8_t *packet) {
    // 取得 pcap handle
    pcap_t *pcap_handle = (pcap_t *)args;

    // 解析 IP 層
    const struct my_iphdr *ip_header = (struct my_iphdr *)(packet + sizeof(struct ether_header));
    int ip_header_len = ip_header->ihl * 4;

    // 只處理 TCP 和 UDP
    if (ip_header->protocol != IPPROTO_TCP && ip_header->protocol != IPPROTO_UDP) {
        return;
    }

    FlowKey key = {0}; // 初始化為 0 以避免警告
    key.ip_src = ip_header->saddr;
    key.ip_dst = ip_header->daddr;
    key.protocol = ip_header->protocol;

    // 解析 Transport 層
    if (key.protocol == IPPROTO_TCP) {
        const struct my_tcphdr *tcp_header = (struct my_tcphdr *)(packet + sizeof(struct ether_header) + ip_header_len);
        key.port_src = tcp_header->th_sport;
        key.port_dst = tcp_header->th_dport;
    } else { // UDP
        const struct my_udphdr *udp_header = (struct my_udphdr *)(packet + sizeof(struct ether_header) + ip_header_len);
        key.port_src = udp_header->uh_sport;
        key.port_dst = udp_header->uh_dport;
    }

    normalize_flow_key(&key); // 正規化 Key

    Flow *found_flow = NULL;
    HASH_FIND(hh, g_flows, &key, sizeof(FlowKey), found_flow);

    if (found_flow) { // Flow 已存在
        // 更新狀態
        found_flow->last_seen = header->ts;
        found_flow->packet_count++;

        // 寫入封包
        pcap_dump((uint8_t *)found_flow->pcap_dumper, header, packet);

        // 檢查是否達到封包數量上限
        if (found_flow->packet_count >= MAX_PACKETS_PER_FLOW) {
            printf("Flow reached packet limit: %s\n", found_flow->filename);
            close_and_free_flow(found_flow, &g_flows);
        }
    } else { // 新的 Flow
        Flow *new_flow = (Flow *)malloc(sizeof(Flow));
        if (!new_flow) {
            perror("Failed to allocate memory for new flow");
            return;
        }
        memcpy(&new_flow->key, &key, sizeof(FlowKey));
        new_flow->packet_count = 1;
        new_flow->last_seen = header->ts;

        // 產生檔名並開啟 pcap dumper
        generate_pcap_filename(&key, new_flow->filename, sizeof(new_flow->filename));
        new_flow->pcap_dumper = pcap_dump_open(pcap_handle, new_flow->filename);
        if (new_flow->pcap_dumper == NULL) {
            fprintf(stderr, "Error opening pcap dump file %s: %s\n", new_flow->filename, pcap_geterr(pcap_handle));
            free(new_flow);
            return;
        }

        printf("New flow created: %s\n", new_flow->filename);

        // 寫入第一個封包
        pcap_dump((uint8_t *)new_flow->pcap_dumper, header, packet);

        // 加入到 Hash Table
        HASH_ADD(hh, g_flows, key, sizeof(FlowKey), new_flow);
    }
}