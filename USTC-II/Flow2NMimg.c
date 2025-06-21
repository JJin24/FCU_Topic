#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <dirent.h>
#include <limits.h>

#define MAX_PKT_NUM 2048 // 可根據需求調整

// 從 pcap 檔案中擷取前 n 個 packet，回傳 packet 指標陣列及實際數量
int extract_n_packets_from_pcap(const char *pcap_filename, int n, const uint8_t **pkt_arr, int *pkt_lens) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_offline(pcap_filename, errbuf);
    if (!handle) {
        fprintf(stderr, "Couldn't open pcap file %s: %s\n", pcap_filename, errbuf);
        return 0;
    }
    struct pcap_pkthdr *header;
    const uint8_t *data;
    int count = 0;
    while (count < n && pcap_next_ex(handle, &header, &data) == 1) {
        pkt_arr[count] = malloc(header->caplen);
        memcpy((uint8_t*)pkt_arr[count], data, header->caplen);
        pkt_lens[count] = header->caplen;
        count++;
    }
    pcap_close(handle);
    return count;
}

uint8_t* get_n_byte_from_pkt(int n, const uint8_t *pkt) {
    uint8_t *n_byte_data = malloc(n);
    if (!n_byte_data) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    memcpy(n_byte_data, pkt, n);

    // 如果 packet 長度小於 n，則填充剩餘部分為 0
    for (int i = strlen((const char*)pkt); i < n; ++i) {
        n_byte_data[i] = 0; // 填充為 0
    }

    return n_byte_data;
}

// 將 n byte 的資料轉換成 one-hot encoded image (256 x n)
/* 這一邊由於 n 相對於 image 先宣告，所以可以使用 image[256][n] (C99 以後的標準) */
void bytes_to_onehot_image(const uint8_t *bytes, int n, unsigned char image[256][n]) {
    memset(image, 0, sizeof(unsigned char) * 256 * n); // 初始化 image 為 0
    for (int i = 0; i < n; ++i) {
        image[bytes[i]][i] = 255; // 灰階最大值
    }
}

// 輸出 image 為 PGM 格式
void save_image_pgm(const char *filename, int cols, unsigned char image[256][cols]) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("fopen");
        return;
    }
    fprintf(fp, "P5\n%d 256\n255\n", cols);
    for (int i = 0; i < 256; ++i) {
        fwrite(image[i], 1, cols, fp);
    }
    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <pcap_folder> <output_img_folder> <n_packets> <n_bytes>\n", argv[0]);
        return 1;
    }
    const char *input_folder = argv[1];
    const char *output_folder = argv[2];
    int n_packets = atoi(argv[3]);
    int n_bytes = atoi(argv[4]);

    DIR *input_dir = opendir(input_folder);
    if (!input_dir) {
        perror("opendir");
        return 1;
    }
    struct dirent *entry;
    char source_filepath[PATH_MAX];
    while ((entry = readdir(input_dir)) != NULL) {
        if (strstr(entry->d_name, ".pcap")) {
            snprintf(source_filepath, sizeof(source_filepath), "%s/%s", input_folder, entry->d_name);
            const uint8_t *pkt_arr[MAX_PKT_NUM];
            int pkt_lens[MAX_PKT_NUM];
            int pkt_count = extract_n_packets_from_pcap(source_filepath, n_packets, pkt_arr, pkt_lens);
            for (int i = 0; i < pkt_count; ++i) {
                int extract_len = n_bytes < pkt_lens[i] ? n_bytes : pkt_lens[i];
                uint8_t *n_byte_data = get_n_byte_from_pkt(extract_len, pkt_arr[i]);
                unsigned char image[256][extract_len];
                bytes_to_onehot_image(n_byte_data, extract_len, image);

                char output_img_path_and_name[512];
                snprintf(output_img_path_and_name, sizeof(output_img_path_and_name), "%s/%s_pkt%d.pgm", output_folder, entry->d_name, i);
                save_image_pgm(output_img_path_and_name, extract_len, image);

                free(n_byte_data);
                free((void*)pkt_arr[i]);
            }
        }
    }
    closedir(input_dir);
    return 0;
}