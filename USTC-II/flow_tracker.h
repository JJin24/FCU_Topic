#ifndef FLOW_TRACKER_H
#define FLOW_TRACKER_H

#include <pcap.h>
#include <time.h>
#include <netinet/in.h>
#include "uthash.h" // 引入 uthash

// --- 可配置常數 ---
#define MAX_PACKETS_PER_FLOW 50     // 每個 Flow 最多收集多少個封包
#define FLOW_TIMEOUT_SECONDS 30     // Flow 的逾時時間 (秒)
#define PCAP_READ_TIMEOUT_MS 1000   // pcap_next_ex 的讀取逾時 (毫秒)，用於觸發逾時檢查

// 5-tuple Key，用於 Hash Table
typedef struct {
    uint32_t ip_src;
    uint32_t ip_dst;
    uint16_t port_src;
    uint16_t port_dst;
    uint8_t protocol;
} FlowKey;

// Flow 的詳細資料結構
typedef struct {
    FlowKey key;                 // 作為 Hash 的 Key
    pcap_dumper_t *pcap_dumper;  // pcap 檔案寫入器
    char filename[256];          // 儲存的檔名
    uint32_t packet_count;       // 封包計數
    struct timeval last_seen;    // 最後看到封包的時間
    UT_hash_handle hh;           // 讓這個結構可以被 uthash 管理
} Flow;

// --- 函式原型宣告 ---

// 處理單一封包的 callback
void process_packet(uint8_t *args, const struct pcap_pkthdr *header, const uint8_t *packet);

// 檢查並移除超時的 Flow
void check_flow_timeouts(Flow **flows_hash);

// 安全地關閉並釋放一個 Flow
void close_and_free_flow(Flow *flow, Flow **flows_hash);

#endif