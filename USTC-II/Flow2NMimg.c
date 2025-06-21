#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <dirent.h>
#include <limits.h>

// 為建立資料夾新增的標頭檔
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define MAX_PKT_NUM 2048 // 可根據需求調整

/**
 * @brief 遞迴地建立資料夾，類似於 `mkdir -p`
 * 
 * @param path 要建立的資料夾路徑 (可以是相對或絕對路徑)
 * @return int 0 代表成功, -1 代表失敗
 */
int create_directories(const char *path) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    // 複製路徑字串，因為我們需要修改它
    snprintf(tmp, sizeof(tmp),"%s", path);
    len = strlen(tmp);

    // 如果路徑以 '/' 結尾，先移除它
    if (len > 1 && tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    // 逐層建立資料夾
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0; // 暫時將 '/' 替換為字串結束符
            // 嘗試建立該層的資料夾
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                perror("mkdir");
                return -1;
            }
            *p = '/'; // 還原 '/'
        }
    }

    // 建立最後一層資料夾
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        perror("mkdir");
        return -1;
    }

    return 0;
}


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

uint8_t* get_n_byte_from_pkt(int n, const uint8_t *pkt, int pkt_len) {
    uint8_t *n_byte_data = malloc(n);
    if (!n_byte_data) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    
    // 決定要複製的實際長度
    int copy_len = (pkt_len < n) ? pkt_len : n;
    memcpy(n_byte_data, pkt, copy_len);

    // 如果 packet 長度小於 n，則填充剩餘部分為 0
    if (copy_len < n) {
        memset(n_byte_data + copy_len, 0, n - copy_len);
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

    // 檢查並建立輸出資料夾
    printf("Checking and creating output directory: %s\n", output_folder);
    if (create_directories(output_folder) != 0) {
        fprintf(stderr, "Error: Could not create output directory '%s'.\n", output_folder);
        return 1; // 建立失敗，結束程式
    }

    DIR *input_dir = opendir(input_folder);
    if (!input_dir) {
        perror("opendir (input)");
        return 1;
    }
    struct dirent *entry;
    char source_filepath[PATH_MAX];
    int deal_pkt = 0; // 用來計算處理的 packet 數量
    while ((entry = readdir(input_dir)) != NULL) {
        if (strstr(entry->d_name, ".pcap")) {
            snprintf(source_filepath, sizeof(source_filepath), "%s/%s", input_folder, entry->d_name);
            const uint8_t *pkt_arr[MAX_PKT_NUM];
            int pkt_lens[MAX_PKT_NUM];
            int pkt_count = extract_n_packets_from_pcap(source_filepath, n_packets, pkt_arr, pkt_lens);
            
            for (int i = 0; i < pkt_count; ++i) {
                // 注意：這裡直接傳入 n_bytes，讓 get_n_byte_from_pkt 內部處理長度與填充
                uint8_t *n_byte_data = get_n_byte_from_pkt(n_bytes, pkt_arr[i], pkt_lens[i]);
                if (!n_byte_data) continue; // 如果記憶體分配失敗則跳過

                unsigned char image[256][n_bytes];
                bytes_to_onehot_image(n_byte_data, n_bytes, image);

                char output_img_path_and_name[PATH_MAX];
                snprintf(output_img_path_and_name, sizeof(output_img_path_and_name), "%s/%s_pkt%d.pgm", output_folder, entry->d_name, i);
                save_image_pgm(output_img_path_and_name, n_bytes, image);

                free(n_byte_data);
                free((void*)pkt_arr[i]);
            }
        }
        if (deal_pkt % 100 == 0) printf("Total %d packets\n", deal_pkt);
    }
    closedir(input_dir);
    printf("Processing complete. Total process %d packets\n", deal_pkt);
    return 0;
}