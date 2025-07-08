#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pcap.h>
#include <stdint.h>
#include "flow_tracker.h"

// 在 main.c 中宣告全域變數的實體
extern Flow *g_flows;
pcap_t *pcap_handle;

// Signal handler, 用於 Ctrl+C 退出時的清理
void cleanup(int signo) {
    (void)signo; // 避免未使用參數的警告
    printf("\nCleaning up and exiting...\n");
    if (pcap_handle) {
        pcap_breakloop(pcap_handle); // 停止 pcap_dispatch/loop
    }
}

void final_cleanup() {
    // 關閉所有剩餘的 open flows
    Flow *current_flow, *tmp;
    HASH_ITER(hh, g_flows, current_flow, tmp) {
        close_and_free_flow(current_flow, &g_flows);
    }

    if (pcap_handle) {
        pcap_close(pcap_handle);
    }
    printf("Cleanup complete.\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <interface>\n", argv[0]);
        return 1;
    }

    char *dev = argv[1];
    char errbuf[PCAP_ERRBUF_SIZE];
    struct bpf_program fp;
    char filter_exp[] = "ip and (tcp or udp)"; // 只抓取 IP 流量中的 TCP 或 UDP

    // 設定 Ctrl+C 的處理
    signal(SIGINT, cleanup);

    // 開啟 pcap handle
    pcap_handle = pcap_open_live(dev, BUFSIZ, 1, PCAP_READ_TIMEOUT_MS, errbuf);
    if (pcap_handle == NULL) {
        fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
        return 2;
    }
    printf("Capturing on device: %s\n", dev);

    // 編譯並設定過濾器
    if (pcap_compile(pcap_handle, &fp, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(pcap_handle));
        return 2;
    }
    if (pcap_setfilter(pcap_handle, &fp) == -1) {
        fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(pcap_handle));
        return 2;
    }

    // 進入主迴圈
    printf("Starting capture loop. Press Ctrl+C to stop.\n");
    while (1) {
        struct pcap_pkthdr *header;
        const uint8_t *packet;
        int ret = pcap_next_ex(pcap_handle, &header, &packet);

        if (ret > 0) {
            // 成功抓到封包
            process_packet((uint8_t *)pcap_handle, header, packet);
        } else if (ret == 0) {
            // 讀取逾時，這是檢查 Flow Timeout 的好時機
            check_flow_timeouts(&g_flows);
        } else if (ret == -1) {
            // 發生錯誤
            fprintf(stderr, "Error reading packets: %s\n", pcap_geterr(pcap_handle));
            break;
        } else if (ret == -2) {
            // pcap_breakloop() 被呼叫，通常是收到 Ctrl+C
            printf("Loop broken.\n");
            break;
        }
    }

    final_cleanup();
    return 0;
}