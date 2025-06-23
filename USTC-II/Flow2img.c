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
//#define DEBUG

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

/*
 * 從 pcap 檔案中擷取前 n 個 packet，回傳 packet 指標陣列及實際數量及每個 packet 的長度。
 * input:
 *   - pcap_filename: pcap 檔案名稱
 *   - n: 要擷取的 packet 數量
 *   - pkt_arr: 指向每個 packet 的指標陣列
 *   - pkt_lens: 每個 packet 的長度陣列
 * return value:
 *   - 回傳實際擷取的 pkt 數量
 *   - pkt_arr: 指向每個 packet 的指標陣列
 *   - pkt_lens: 每個 pkt 的長度陣列
 */
unsigned int extract_n_packets_from_pcap(const char *pcap_filename, int n, const uint8_t **pkt_arr, int *pkt_lens) {
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

/*
 * 從 pcap 檔案中連續讀取封包，直到填滿 n 個位元組到 output_buffer。
 * 回傳實際讀取的位元組數量。
 * input:
 *   - n: 要讀取的位元組數量
 *   - pcap_filename: pcap 檔案名稱
 *   - output_buffer: 用來存放讀取的位元組的 buffer
 *   - pkt_lens: 用來存放每個封包的長度
 * return value:
 *   - 回傳實際讀取的位元組數量
 *   - output_buffer: 包含讀取的位元組
 */
unsigned int get_n_byte_from_flow(int n, const char *pcap_filename, uint8_t* output_buffer) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_offline(pcap_filename, errbuf);
    if (!handle) {
        fprintf(stderr, "Couldn't open pcap file %s: %s\n", pcap_filename, errbuf);
        return 0;
    }
    
    struct pcap_pkthdr *header;
    const uint8_t *packet_data;
    int bytes_copied = 0; // 已複製的總位元組數

    while (bytes_copied < n && pcap_next_ex(handle, &header, &packet_data) == 1) {
        int bytes_to_copy;
        unsigned int remaining_space = n - bytes_copied; // 緩衝區剩餘空間

        // 決定這次要複製多少位元組：取「封包長度」和「緩衝區剩餘空間」的較小值
        if (header->caplen < remaining_space) {
            bytes_to_copy = header->caplen;
        }
        else {
            bytes_to_copy = remaining_space;
        }

        // 從封包資料複製到我們的輸出緩衝區
        memcpy(output_buffer + bytes_copied, packet_data, bytes_to_copy);
        
        // 更新已複製的總位元組數
        bytes_copied += bytes_to_copy;
    }
    
    pcap_close(handle);
    
    return bytes_copied;
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

/*
 * 為符合 USTC-II 的要求，將 n byte 的資料轉換成 one-hot encoded image (256 x n)
 * 對 image 填入的數值如下
 * (255, 0), (255, 1), ..., (255, n)
 * (254, 0), (254, 1), ..., (254, n)
 * ...
 * ....
 * ....
 * (0, 0), (0, 1), ..., (0, n)
 */
void bytes_to_onehot_image(const uint8_t *bytes, int n, unsigned char image[256][n], int current_get_bytes) {
    memset(image, 0, sizeof(unsigned char) * 256 * n); // 初始化 image 為 0

    /* 下面這一段將會像是 wireshark 的形式顯示 bytes 中的內容 */
    #ifdef DEBUG
    for(int i = 0; i < n ; i++){
        printf("%.2x ", bytes[i]);
        if (i % 8 == 7) printf(" ");
        if (i % 16 == 15) printf("\n");
    }
    #endif

    for (int i = 0; i < current_get_bytes; ++i) {
        image[255 - bytes[i]][i] = 255; // 灰階最大值
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

/*
 * 填充成 PGM 格式的圖片，格式為：
 *
 * P5
 * n 256
 * 255
 * image data (每行 n bytes)
 */
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

void help (const char *program_name){
    fprintf(stdout, "Usage:\n");
    fprintf(stdout, "  [OPTION-1] = -1, -f, --flow, --hast-1, --HAST-I\n");
    fprintf(stdout, "    %s [OPTION-1] <pcap_folder> <output_img_folder> <m_bytes>\n", program_name);
    fprintf(stdout, "    使用 HAST-I 的方式提取 Flow 的前 m 位元組\n\n");
    fprintf(stdout, "  [OPTION-2] = -2, -p, --packet, --hast-2, --HAST-II\n");
    fprintf(stdout, "    %s [OPTION-2] <pcap_folder> <output_img_folder> <n_packets> <m_byte>\n", program_name);
    fprintf(stdout, "    使用 HAST-II 的方式先提取 Flow 的前 n 個封包，接著在每個封包中提取前 m 位元\n\n");
    fprintf(stdout, "  [OPTION-3] = -h, --help\n");
    fprintf(stdout, "    %s [OPTION-3]\n", program_name);
    fprintf(stdout, "    顯示使用方法\n\n");
    fprintf(stdout, "論文連結:https://ieeexplore.ieee.org/document/8171733\n");
}

int HAST_ONE(const char *pcap_folder, const char *output_img_folder, const char *m_bytes_str) {
    int n_bytes = atoi(m_bytes_str);
    if (n_bytes <= 0) {
        fprintf(stderr, "Invalid number of packets or bytes: %s\n", m_bytes_str);
        return 1;
    }
    
    if (create_directories(output_img_folder) != 0) {
        fprintf(stderr, "Error: Could not create output directory '%s'.\n", output_img_folder);
        return 1; // 建立失敗，結束程式
    }

    /* 開啟資料夾 */
    DIR *input_dir = opendir(pcap_folder);
    if (!input_dir) {
        perror("opendir (input)");
        return 1;
    }
    struct dirent *entry;
    char source_filepath[PATH_MAX];
    int deal_file_count = 0;
    
    while ((entry = readdir(input_dir)) != NULL) {
        uint8_t* top_n_byte_data = malloc(sizeof(uint8_t) * n_bytes);
        if(top_n_byte_data == NULL){
            perror("HAST-I malloc error");
        }

        memset(top_n_byte_data, 0, n_bytes);
        int current_get_bytes = 0;

        /* 當有 .pcap 檔名的檔案的時候才會開始處理 */
        if (strstr(entry->d_name, ".pcap")) {
            /* 在 pcap 中抓前 n byte */
            snprintf(source_filepath, sizeof(source_filepath), "%s/%s", pcap_folder, entry->d_name);
            current_get_bytes = get_n_byte_from_flow(n_bytes ,source_filepath, top_n_byte_data);

            /* 轉換成圖片 */
            unsigned char image[256][n_bytes];
            bytes_to_onehot_image(top_n_byte_data, n_bytes, image, current_get_bytes);
            char output_img_path_and_name[PATH_MAX];
            snprintf(output_img_path_and_name, sizeof(output_img_path_and_name), "%s/%s.pgm", output_img_folder, entry->d_name);
            save_image_pgm(output_img_path_and_name, n_bytes, image);
            free(top_n_byte_data);

            if(deal_file_count++ % 10 == 0) printf("Process %d flows\r", deal_file_count - 1);
        }
    }
    closedir(input_dir);
    printf("Processing complete. Total process %d flows\n", deal_file_count);
    return 0;
}

int HAST_TWO(const char *pcap_folder, const char *output_img_folder, const char *n_packets_str, const char *n_bytes_str) {
    int n_packets = atoi(n_packets_str);
    int n_bytes = atoi(n_bytes_str);
    if (n_packets <= 0 || n_bytes <= 0) {
        fprintf(stderr, "Invalid number of packets or bytes: %s, %s\n", n_packets_str, n_bytes_str);
        return 1;
    }

    if (n_packets >= 4095){
        fprintf(stderr, "Warning: n_packets is too large, it may cause memory issues.\n");
        return 1; // 如果 n_packets 太大，直接結束程式
    }

    // 檢查並建立輸出資料夾
    printf("Checking and creating output directory: %s\n", output_img_folder);
    if (create_directories(output_img_folder) != 0) {
        fprintf(stderr, "Error: Could not create output directory '%s'.\n", output_img_folder);
        return 1; // 建立失敗，結束程式
    }

    DIR *input_dir = opendir(pcap_folder);
    if (!input_dir) {
        perror("opendir (input)");
        return 1;
    }
    struct dirent *entry;
    char source_filepath[PATH_MAX];
    int deal_pkt = 0; // 用來計算處理的 packet 數量
    while ((entry = readdir(input_dir)) != NULL) {
        if (strstr(entry->d_name, ".pcap")) {
            snprintf(source_filepath, sizeof(source_filepath), "%s/%s", pcap_folder, entry->d_name);
            const uint8_t *pkt_arr[MAX_PKT_NUM];
            int pkt_lens[MAX_PKT_NUM];
            int pkt_count = extract_n_packets_from_pcap(source_filepath, n_packets, pkt_arr, pkt_lens);

            /* 替每一個 flow 建立一個新的資料夾 */
            char output_flow_img_folder[PATH_MAX];
            snprintf(output_flow_img_folder, sizeof(output_flow_img_folder), "%s/%s", output_img_folder, entry->d_name);
            if (create_directories(output_flow_img_folder) != 0) {
                fprintf(stderr, "Error: Could not create output directory '%s'.\n", output_flow_img_folder);
                continue; // 建立失敗，跳過這個 packet
            }
            
            for (int i = 0; i < pkt_count; ++i) {
                // 注意：這裡直接傳入 n_bytes，讓 get_n_byte_from_pkt 內部處理長度與填充
                uint8_t *n_byte_data = get_n_byte_from_pkt(n_bytes, pkt_arr[i], pkt_lens[i]);
                if (!n_byte_data) continue; // 如果記憶體分配失敗則跳過

                unsigned char image[256][n_bytes];
                bytes_to_onehot_image(n_byte_data, n_bytes, image, pkt_lens[i]);

                char output_img_path_and_name[PATH_MAX];

                int written_len = snprintf(output_img_path_and_name, sizeof(output_img_path_and_name), "%s/%d.pgm", output_flow_img_folder, i);

                // Check if the output was truncated or if an error occurred
                if (written_len < 0 || written_len >= (int)sizeof(output_img_path_and_name)) {
                    fprintf(stderr, "Error: Output path is too long for the buffer. Skipping file.\n");
                    // Clean up and continue to the next iteration
                    free(n_byte_data);
                    free((void*)pkt_arr[i]);
                    continue; 
                }

                save_image_pgm(output_img_path_and_name, n_bytes, image);

                free(n_byte_data);
                free((void*)pkt_arr[i]);
            }

            /* 空圖片 (資料準備) */
            unsigned char image[256][n_bytes];
            memset(image, 0, sizeof(image)); // 清空圖片

            /* 填充空圖片 */
            for (int i = pkt_count; i < n_packets; ++i) {
                char output_img_path_and_name[PATH_MAX];
                int written_len = snprintf(output_img_path_and_name, sizeof(output_img_path_and_name), "%s/%d.pgm", output_flow_img_folder, i);
                
                // Check if the output was truncated or if an error occurred
                if (written_len < 0 || written_len >= (int)sizeof(output_img_path_and_name)) {
                    fprintf(stderr, "Error: Output path is too long for the buffer. Skipping file.\n");
                    continue;
                }

                save_image_pgm(output_img_path_and_name, n_bytes, image);
            }

            if (deal_pkt++ % 10 == 0) printf("Process %d packets\r", deal_pkt - 1);
        }

    }
    closedir(input_dir);
    printf("Processing complete. Total process %d packets\n", deal_pkt);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 6) {
        help(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        help(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "-f") == 0 || strcmp(argv[1], "--flow") == 0 || strcmp(argv[1], "-1") == 0 || strcmp(argv[1], "--hast-1" ) == 0 || strcmp(argv[1], "--HAST-I" ) == 0){
        if (argc != 5) {
            printf("Usage: %s -f <pcap_folder> <output_img_folder> <n_bytes>\n", argv[0]);
            return 1;
        }
        int status = HAST_ONE(argv[2], argv[3], argv[4]);

        if (status != 0) return status;
        return 0;
    }

    if (strcmp(argv[1], "-p") == 0 || strcmp(argv[1], "--packet") == 0 || strcmp(argv[1], "-2") == 0 || strcmp(argv[1], "--hast-2" ) == 0 || strcmp(argv[1], "--HAST-II" ) == 0){
        if (argc != 6) {
            printf("Usage: %s -p <pcap_folder> <output_img_folder> <n_packets> <n_byte>\n", argv[0]);
            return 1;
        }
        int status = HAST_TWO(argv[2], argv[3], argv[4], argv[5]);

        if (status != 0)return status;
        return 0;
    }

    help(argv[0]);
    return 1;
}