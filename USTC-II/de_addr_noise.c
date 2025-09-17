#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/ether.h>

#include "de_addr_noise.h"

//#define DEBUG

int de_ipv4(const uint8_t *bytes, int n, unsigned char image[256][n], int i) {
    struct iphdr *ip_header = (struct iphdr *)(bytes + i);

    /* 傳給 IPv4 Header 長度不固定，傳目前已處理的數量 (+ Layer 2) */
    int next_i = i + ip_header->ihl * 4;

    int top_i = i;

    /* ONE HOT IPv4 Header */
    for(; i < next_i; ++i) {
        /* 跳過 IP Header 的 IP 部分 */
        if(i > top_i + 12 - 1 && i <= (int)(top_i + 12 + sizeof(ip_header->saddr) * 2)) {
            continue;
        }

        image[255 - bytes[i]][i] = 255; // 灰階最大值
    }

    return next_i;
}

int de_ipv6(const uint8_t *bytes, int n, unsigned char image[256][n], int i) {
    struct ip6_hdr *ip_header = (struct ip6_hdr *)(bytes + i);

    /* 傳 IPv6 Header 的長度 40 Bytes (+ Layer 2) */
    int next_i = i + 40;

    int top_i = i;
    /* ONE HOT IPv6 Header */
    for(; i < next_i; ++i) {
        /* 跳過 IP Header 的 IP 部分 */
        if(i > top_i + 8 - 1 && i <= (int)(top_i + 8 + sizeof(ip_header->ip6_src) * 2)) {
            continue;
        }

        image[255 - bytes[i]][i] = 255; // 灰階最大值
    }

    return next_i;
}

int de_addr(const uint8_t *bytes, int n, unsigned char image[256][n], int i) {
    struct ether_header *eth_header = (struct ether_header *)bytes;
    int next_i = sizeof(struct ether_header); /* 傳給 Layer 3 目前已處理的數量 */

    uint16_t ether_type = ntohs(eth_header->ether_type);

    /*
     * 由於 VLAN 也屬於 Layer 2，並且其 Header 附在 ether_header 後面
     * 因此將看到的 ether_type 為 ETHERTYPE_8021Q (VLAN) 時，跳過 VLAN header
     */
    if(ether_type == ETHERTYPE_VLAN){
        next_i += sizeof(struct vlan_hdr);

        struct vlan_hdr *vlan_header = 
        (struct vlan_hdr *)(bytes + sizeof(struct ether_header));

        ether_type = vlan_header->h_vlan_encapsulated_proto;
    }

    /* 複製 MAC Addr 後面的到 Layer 3 的開始 */
    for(i += (sizeof(eth_header->ether_shost) * 2); i < next_i; ++i) {
        image[255 - bytes[i]][i] = 255; // 灰階最大值
    }

    switch (ether_type) {
        case ETHERTYPE_IP: {
            /* 處理 IPv4 */
            i = de_ipv4(bytes, n, image, i);
            break;
        }
        case ETHERTYPE_IPV6: {
            /* 處理 IPv6 */
            i = de_ipv6(bytes, n, image, i);
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
 * 將 bytes 轉換到 ONE HOT 編碼的圖片格式，並且會轉換的圖片刪除 Addr 相關的資訊
 */
void __DENOISE_bytes_to_onehot_image(const uint8_t *bytes, int n, 
    unsigned char image[256][n], int current_get_bytes) {
    memset(image, 0, sizeof(unsigned char) * 256 * n); // 初始化 image 為 0

    /* 下面這一段將會像是 wireshark 的形式顯示 bytes 中的內容 */
    #ifdef DEBUG
    for(int i = 0; i < n - 1 ; i++){
        printf("%.2x ", bytes[i]);
        if (i % 8 == 7) printf(" ");
        if (i % 16 == 15) printf("\n");
    }
    printf("%d\n", bytes[current_get_bytes]);
    #endif

    /* 
     * 當 pcap 抓取到 bytes 大於實際所需時，僅須複製實際所需要的大小即可
     * 以避免 stack overflow 的問題
     */
    if (current_get_bytes > n){
        int i = 0;
        i = de_addr(bytes, n, image, i);

        /* 處理剩餘部分 */
        for (; i < n; ++i) {
            image[255 - bytes[i]][i] = 255;
        }
    }

    /*
     * 當 pcap 抓取到的 bytes 小於實際所需時，僅會複製到實際所需的大小
     */
    else{
        int i = 0;
        i = de_addr(bytes, n, image, i);

        /* 處理剩餘部分 */
        for (; i < current_get_bytes; ++i) {
            image[255 - bytes[i]][i] = 255; // 灰階最大值
        }
    }

    /* 下面這一個可以將程式的輸出使用 .csv 開啟，驗證圖片的輸出 */
    #ifdef DEBUG
    for (int i = 255; i >= 0; i--) {
        for (int j = 0; j < n; ++j) {
            printf("%d ,", image[i][j]);
        }
        printf("\n");
    }
    #endif
}

void de_ipv4_gray(uint8_t* image, int i) {
    struct iphdr *ip_header = (struct iphdr *)(image + i);

    //傳給 IPv4 Header 長度不固定，傳目前已處理的數量 (+ Layer 2)
    // int next_i = i + ip_header->ihl * 4;

    /*
     * image + i (到 IP Header)
     * + 12 - 1(到 IPv4 Address 的頭)
     * sizeof(ip_header->daddr) * 2 (Daddr + Saddr 的長度)
     */
    memset(image + i + 12, 0, sizeof(ip_header->daddr) * 2);

    return;
}

void de_ipv6_gray(uint8_t* image, int i) {
    struct ip6_hdr *ip_header = (struct ip6_hdr *)(image + i);

    //傳給 IPv4 Header 長度不固定，傳目前已處理的數量 (+ Layer 2)
    // int next_i = i + ip_header->ihl * 4;

    /*
     * image + i (到 IP Header)
     * + 8 - 1(到 IPv6 Address 的頭)
     * sizeof(ip_header->ip6_dst) * 2 (Daddr + Saddr 的長度)
     */
    memset(image + i + 8, 0, sizeof(ip_header->ip6_dst) * 2);

    return;
}

void de_addr_grays(uint8_t *image){
    struct ether_header *eth_header = (struct ether_header *)image;
    int next_i = sizeof(struct ether_header); /* 傳給 Layer 3 目前已處理的數量 */

    uint16_t ether_type = ntohs(eth_header->ether_type);

    /*
     * 由於 VLAN 也屬於 Layer 2，並且其 Header 附在 ether_header 後面
     * 因此將看到的 ether_type 為 ETHERTYPE_8021Q (VLAN) 時，跳過 VLAN header
     */
    if(ether_type == ETHERTYPE_VLAN){
        next_i += sizeof(struct vlan_hdr);

        struct vlan_hdr *vlan_header = 
        (struct vlan_hdr *)(image + sizeof(struct ether_header));

        ether_type = vlan_header->h_vlan_encapsulated_proto;
    }
    // 將 MAC Address 的灰階影像設定為 0
    memset(image, 0, (sizeof(eth_header->ether_shost) * 2));

    switch (ether_type) {
        case ETHERTYPE_IP: {
            /* 處理 IPv4 */
            de_ipv4_gray(image, next_i);
            break;
        }
        case ETHERTYPE_IPV6: {
            /* 處理 IPv6 */
            de_ipv6_gray(image, next_i);
            break;
        }
        default: {
            /* 處理其他 Layer 3 協定 */
            break;
        }
    }
}

/*
 * 將 bytes 轉換到 ONE HOT 編碼的圖片格式，並且會轉換的圖片刪除 Addr 相關的資訊
 */
void __DENOISE_bytes_Grayscale_image(const uint8_t *bytes, int n, int row,
	unsigned char image[row][16], int current_get_bytes) {
    memset(image, 0, sizeof(unsigned char) * 16 * row); // 初始化 image 為 0

    /* 下面這一段將會像是 wireshark 的形式顯示 bytes 中的內容 */
    #ifdef DEBUG
    for(int i = 0; i < n - 1 ; i++){
        printf("%.2x ", bytes[i]);
        if (i % 8 == 7) printf(" ");
        if (i % 16 == 15) printf("\n");
    }
    printf("%d\n", bytes[current_get_bytes]);
    #endif

    /* 
     * 當 pcap 抓取到 bytes 大於實際所需時，僅須複製實際所需要的大小即可
     * 以避免 stack overflow 的問題
     */
    if (current_get_bytes > n){
        // 直接將 n Bytes 先複製到 image 中
        if(memcpy(image, bytes, n) != image) perror("Can't Create a new image.");

        de_addr_grays((uint8_t *)image);
        
    }

    /*
     * 當 pcap 抓取到的 bytes 小於實際所需時，僅會複製到實際所需的大小
     */
    else{
        // 將實際上得到的 Bytes 數複製到 image 中即可
        if(memcpy(image, bytes, current_get_bytes) != image) {
            perror("Can't Create a new image.");
        }

        de_addr_grays((uint8_t *)image);
    }

    /* 下面這一個可以將程式的輸出使用 .csv 開啟，驗證圖片的輸出 */
    #ifdef DEBUG
    for (int i = 255; i >= 0; i--) {
        for (int j = 0; j < n; ++j) {
            printf("%d ,", image[i][j]);
        }
        printf("\n");
    }
    #endif
}