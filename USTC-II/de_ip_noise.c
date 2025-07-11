#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/ether.h>

int de_ipv4(const uint8_t *bytes, unsigned char image[256][n], int i) {
    struct iphdr *ip_header = (struct iphdr *)(bytes + i);

    /* 傳給 IPv4 Header 長度不固定，傳目前已處理的數量 (+ Layer 2) */
    int next_i = i + ip_header->ihl * 4;

    int top_i = i;
    /* 複製 IP Addr 後面的到 Layer 3 的開始 */
    for(; i < next_i; ++i) {
        /* 跳過 IP Header 的 Source IP 部分 */
        if(i < top_i + 12 && i >= top_i + 12 +sizeof(ip_header->saddr)) {
            continue;
        }

        image[255 - bytes[i]][i] = 255; // 灰階最大值
    }

    return next_i;
}

int de_ipv6(const uint8_t *bytes, unsigned char image[256][n], int i) {
    struct ip6_hdr *ip_header = (struct ip6_hdr *)(bytes + i);

    /* 傳 IPv6 Header 的長度 40 Bytes (+ Layer 2) */
    int next_i = i + 40;

    int top_i = i;
    /* 複製 IP Addr 後面的到 Layer 3 的開始 */
    for(; i < next_i; ++i) {
        /* 跳過 IP Header 的 Source IP 部分 */
        if(i < top_i + 8 && i >= top_i + 8 +sizeof(ip_header->saddr)) {
            continue;
        }

        image[255 - bytes[i]][i] = 255; // 灰階最大值
    }

    return next_i;
}

int de_addr(const uint8_t *bytes, unsigned char image[256][n], int i) {
    struct ether_header *eth_header = (struct ether_header *)bytes;
    int next_i = sizeof(struct ether_header); /* 傳給 Layer 3 目前已處理的數量 */

    /*
     * 由於 VLAN 也屬於 Layer 2，並且其 Header 附在 ether_header 後面
     * 因此將看到的 ether_type 為 ETHERTYPE_8021Q (VLAN) 時，跳過 VLAN header
     */
    if(eth_header->ether_type == ntohs(ETHERTYPE_8021Q)){
        next_i += sizeof(struct ether_vlan_header);

        struct ether_vlan_header *vlan_header = \
        (struct ether_vlan_header *)(bytes + sizeof(struct ether_header));

        eth_header->ether_type = vlan_header->ether_type;
    }

    /* 複製 MAC Addr 後面的到 Layer 3 的開始 */
    for(i += (sizeof(eth_header->ether_shost) * 2); i < next_i; ++i) {
        image[255 - bytes[i]][i] = 255; // 灰階最大值
    }

    switch (eth_header->ether_type) {
        case ntohs(ETHERTYPE_IP): {
            /* 處理 IPv4 */
            i = de_ipv4(bytes, image, i);
            break;
        }
        case ntohs(ETHERTYPE_IPV6): {
            /* 處理 IPv6 */
            i = de_ipv6(bytes, image, i);
            break;
        }
        default: {
            /* 處理其他 Layer 3 協定 */
            break;
        }    
    }
    return i;
}


/*
 * 將 bytes 轉換到 one-hot 編碼的圖片格式，並且會轉換的圖片刪除 Address 資訊
 */
void __DENOISE_bytes_to_onehot_image(const uint8_t *bytes, int n, 
    unsigned char image[256][n], int current_get_bytes) {
    memset(image, 0, sizeof(unsigned char) * 256 * n); // 初始化 image 為 0

    /* 下面這一段將會像是 wireshark 的形式顯示 bytes 中的內容 */
    #ifdef DEBUG
    for(int i = 0; i < n ; i++){
        printf("%.2x ", bytes[i]);
        if (i % 8 == 7) printf(" ");
        if (i % 16 == 15) printf("\n");
    }
    #endif

    /* 
     * 當pcap 抓取到 bytes大於實際所需時，僅須複製實際所需要的大小即可
     * 以避免 stack overflow 的問題
     */
    if (current_get_bytes > n){
        for (int i = 0; i < n; ++i) {
            image[255 - bytes[i]][i] = 255; // 灰階最大值
        }

        int i = 0;
        i = de_addr(bytes, image, i);

        /* 處理剩餘部分 */
        for (; i < n; ++i) {
            image[255 - bytes[i]][i] = 255; // 灰階最大值
        }
    }

    /*
     * 當 pcap 抓取到的 bytes 小於實際所需時，僅會複製到實際所需的大小
     */
    else{
        for (int i = 0; i < n; ++i) {
            image[255 - bytes[i]][i] = 255; // 灰階最大值
        }

        int i = 0;
        i = de_addr(bytes, image, i);

        /* 處理剩餘部分 */
        for (; i < current_get_bytes; ++i) {
            image[255 - bytes[i]][i] = 255; // 灰階最大值
        }
    }

    /* 下面這一個可以將程式的輸出使用 .csv 開啟，驗證圖片的輸出 */
    #ifdef DEBUG
    for (i = 255; i >= 0; i--) {
        for (int j = 0; j < n; ++j) {
            printf("%d ,", image[i][j]);
        }
        printf("\n");
    }
    #endif
}