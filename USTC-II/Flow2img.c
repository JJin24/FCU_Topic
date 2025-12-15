#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <dirent.h>
#include <linux/limits.h>

// 為建立資料夾新增的標頭檔
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

/* png 相關的定義 */
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "de_addr_noise.h"

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
void bytes_to_onehot_image(const uint8_t *bytes, int n, uint8_t image[256][n], int current_get_bytes) {
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
     * 當pcap 抓取到 bytes大於實際所需時，僅須複製實際所需要的大小即可
     * 以避免 stack overflow 的問題
     */
    if (current_get_bytes > n){
        for (int i = 0; i < n; ++i) {
            image[255 - bytes[i]][i] = 255; // 灰階最大值
        }
    }

    /*
     * 當 pcap 抓取到的 bytes 小於實際所需時，僅會複製到實際所需的大小
     */
    else{
        for (int i = 0; i < current_get_bytes; ++i) {
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

void save_image_png(const char * filename, int cols, uint8_t image[256][cols]) {
    if(stbi_write_png(filename, cols, 256, 1, image, cols * sizeof(unsigned char)) == 0) {
        fprintf(stderr, "Error saving PNG image to %s\n", filename);
        return;
    }
}

void save_gray_image_png(const char * filename, int row, uint8_t image[row][16]){
    if(stbi_write_png(filename, 16, row, 1, image, 16 * sizeof(unsigned char)) == 0) {
        fprintf(stderr, "Error saving Gray PNG image to %s\n", filename);
        return;
    }
}

/*
 * stbi_write_png 傳入的彩色圖片資料不能為 "平面式" 的陣列 ([channel][hight][width])
 * 而必須為 "交錯式" 的陣列 ([hight][width][channel])，這樣才可以使得輸出的圖片正確
 * 
 * 在這邊修改的原因是因為前面處理資料方式較為複雜，且在處理灰階影像 de_addr_noise 的地方
 * 已經接用用了平面式的陣列 ([hight][width])，為維持程式一致性，因此改成在此轉換
 * 即使處理時間較為複雜
 */ 
void save_color_image_png(const char * filename, int row, uint8_t image[3][row][16]){
    uint8_t image_interleaved[row][16][3];

    for(int i = 0; i < row; i++){
        for(int j = 0; j < 16; j++){
            image_interleaved[i][j][0] = image[0][i][j]; // R
            image_interleaved[i][j][1] = image[1][i][j]; // G
            image_interleaved[i][j][2] = image[2][i][j]; // B
        }
    }

    if(stbi_write_png(filename, 16, row, 3, image_interleaved, 16 * 3 * sizeof(unsigned char)) == 0) {
        fprintf(stderr, "Error saving Color PNG image to %s\n", filename);
        return;
    }
}

void help (const char *program_name){
    fprintf(stdout, "Usage:\n");
    fprintf(stdout, "  [OPTION-1] = -1, -f, --flow, --hast-1, --HAST-I\n");
    fprintf(stdout, "    %s [OPTION-1] <pcap_folder> <output_img_folder> "
                    "<m_bytes> <opt>\n", program_name);
    fprintf(stdout, "    使用 HAST-I 的方式提取 Flow 的前 m 位元組\n\n");
    fprintf(stdout, "  [OPTION-2] = -2, -p, --packet, --hast-2, --HAST-II\n");
    fprintf(stdout, "    %s [OPTION-2] <pcap_folder> <output_img_folder> "
                    "<n_packets> <m_byte> <opt>\n", program_name);
    fprintf(stdout, "    使用 HAST-II 的方式先提取 Flow 的前 n 個封包，接著在每個封包中提取前 m 位元\n\n");
    fprintf(stdout, "  [OPTION-3] = -3\n");
    fprintf(stdout, "    %s [OPTION-3] <pcap_folder> <output_img_folder> "
                    "<n_packets> <m_byte> <opt>\n", program_name);
    fprintf(stdout, "    使用 HAST-II 的方式先提取 Flow 的前 n 個封包，接著在每個封包中 "
                    "提取前 m 位元後轉換成 w: 16, h: m / 16 大小的灰階影像\n\n");
    fprintf(stdout, "  [OPTION-4] = -4\n");
    fprintf(stdout, "    %s [OPTION-4] <pcap_folder> <output_img_folder> "
                    "<n_packets> <m_byte> <opt>\n", program_name);
    fprintf(stdout, "    使用 HAST-II 的方式先提取 Flow 的前 n 個封包，接著在每個封包中 "
                    "每 3 個封包提取前 m 位元後轉換成 w: 16, h: m / 16 大小的彩色影像\n\n");
    fprintf(stdout, "  [OPTION-5] = -h, --help\n");
    fprintf(stdout, "    %s [OPTION-5]\n", program_name);
    fprintf(stdout, "    顯示使用方法\n\n");
    fprintf(stdout, "論文連結:https://ieeexplore.ieee.org/document/8171733\n");
}

int HAST_ONE(const char *pcap_folder, const char *output_img_folder, const char *m_bytes_str, const char *opt) {
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
            uint8_t image[256][n_bytes];

            if (strcmp(opt, "de_addr_noise") == 0) {
                __DENOISE_bytes_to_onehot_image(top_n_byte_data, n_bytes, image,
                current_get_bytes); // 使用去除 IP 的版本
            }
            else {
                bytes_to_onehot_image(top_n_byte_data, n_bytes, image, current_get_bytes);
            }
            
            char output_img_path_and_name[PATH_MAX];
            snprintf(output_img_path_and_name, sizeof(output_img_path_and_name), "%s/%s.png", output_img_folder, entry->d_name);
            save_image_png(output_img_path_and_name, n_bytes, image);
            free(top_n_byte_data);

            if(deal_file_count++ % 10 == 0) printf("Process %d flows\r", deal_file_count - 1);
        }
    }
    closedir(input_dir);
    printf("Processing complete. Total process %d flows\n", deal_file_count);
    return 0;
}

int HAST_TWO(const char *pcap_folder, const char *output_img_folder,\
    const char *n_packets_str, const char *n_bytes_str, const char *opt) {
    printf("%s\n", opt);
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
            const uint8_t *pkt_arr[n_packets]; /* 建立一個 '存放 uint_8 指標' 的陣列 */
            int pkt_lens[n_packets];
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

                uint8_t image[256][n_bytes];

                if (strcmp(opt, "de_addr_noise") == 0) {
                    __DENOISE_bytes_to_onehot_image(n_byte_data, n_bytes, image,
                    pkt_lens[i]); // 使用去除 IP 的版本
                }
                else {
                    bytes_to_onehot_image(n_byte_data, n_bytes, image, pkt_lens[i]);
                }
                char output_img_path_and_name[PATH_MAX];

                int written_len = snprintf(output_img_path_and_name, sizeof(output_img_path_and_name), "%s/%d.png", output_flow_img_folder, i);

                // Check if the output was truncated or if an error occurred
                if (written_len < 0 || written_len >= (int)sizeof(output_img_path_and_name)) {
                    fprintf(stderr, "Error: Output path is too long for the buffer. Skipping file.\n");
                    // Clean up and continue to the next iteration
                    free(n_byte_data);
                    free((void*)pkt_arr[i]);
                    continue; 
                }

                save_image_png(output_img_path_and_name, n_bytes, image);

                free(n_byte_data);
                free((void*)pkt_arr[i]);
            }

            /* 空圖片 (資料準備) */
            uint8_t image[256][n_bytes];
            memset(image, 0, sizeof(unsigned char) * 256 * n_bytes); // 清空圖片

            /* 填充空圖片 */
            for (int i = pkt_count; i < n_packets; ++i) {
                char output_img_path_and_name[PATH_MAX];
                int written_len = snprintf(output_img_path_and_name, sizeof(output_img_path_and_name), "%s/%d.png", output_flow_img_folder, i);
                
                // Check if the output was truncated or if an error occurred
                if (written_len < 0 || written_len >= (int)sizeof(output_img_path_and_name)) {
                    fprintf(stderr, "Error: Output path is too long for the buffer. Skipping file.\n");
                    continue;
                }

                save_image_png(output_img_path_and_name, n_bytes, image);
            }

            if (deal_pkt++ % 10 == 0) printf("Process %d flows\r", deal_pkt - 1);
        }
    }

    closedir(input_dir);
    printf("Processing complete. Total process %d flows\n", deal_pkt);
    return 0;
}

int HAST_Three(const char *pcap_folder, const char *output_img_folder,\
    const char *n_packets_str, const char *n_bytes_str, const char *opt) {
    printf("%s\n", opt);
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

    // 設定 row 的長度
    int row = 0;
    if (n_bytes % 16 != 0){
        row = n_bytes / 16 + 1;
    }
    else row = n_bytes / 16;

    struct dirent *entry;
    char source_filepath[PATH_MAX];
    int deal_pkt = 0; // 用來計算處理的 packet 數量
    while ((entry = readdir(input_dir)) != NULL) {
        if (strstr(entry->d_name, ".pcap")) {
            snprintf(source_filepath, sizeof(source_filepath), "%s/%s", pcap_folder, entry->d_name);
            const uint8_t *pkt_arr[n_packets]; /* 建立一個 '存放 uint_8 指標' 的陣列 */
            int pkt_lens[n_packets];
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

                uint8_t image[row][16];

                if (strcmp(opt, "de_addr_noise") == 0) {
                    __DENOISE_bytes_Grayscale_image(n_byte_data, n_bytes, row, image,
                    pkt_lens[i]); // 使用去除 IP 的版本
                }
                else {
                    memset(image, 0, sizeof(unsigned char) * 16 * row);
                    if (pkt_lens[i] > n_bytes){
                        // 直接將 n Bytes 先複製到 image 中
                        if(memcpy(image, n_byte_data, n_bytes) != image) perror("Can't Create a new image.");
                    }

                    /*
                     * 當 pcap 抓取到的 bytes 小於實際所需時，僅會複製到實際所需的大小
                     */
                    else{
                        // 將實際上得到的 Bytes 數複製到 image 中即可
                        if(memcpy(image, n_byte_data, pkt_lens[i]) != image) {
                            perror("Can't Create a new image.");
                        }
                    }
                }
                char output_img_path_and_name[PATH_MAX];

                int written_len = snprintf(output_img_path_and_name, sizeof(output_img_path_and_name), "%s/%d.png", output_flow_img_folder, i);

                // Check if the output was truncated or if an error occurred
                if (written_len < 0 || written_len >= (int)sizeof(output_img_path_and_name)) {
                    fprintf(stderr, "Error: Output path is too long for the buffer. Skipping file.\n");
                    // Clean up and continue to the next iteration
                    free(n_byte_data);
                    free((void*)pkt_arr[i]);
                    continue; 
                }

                #ifdef DEBUG
                uint8_t* data = image;

                for(int i = 0; i < 16 * row; i++){
                    printf("%.2x ", data[i]);
                    if (i % 8 == 7) printf(" ");
                    if (i % 16 == 15) printf("\n");
                }
                printf("\n");
                #endif

                save_gray_image_png(output_img_path_and_name, row, image);

                free(n_byte_data);
                free((void*)pkt_arr[i]);
            }

            /* 空圖片 (資料準備) */
            uint8_t image[row][16];
            memset(image, 0, sizeof(unsigned char) * row * 16); // 清空圖片

            /* 填充空圖片 */
            for (int i = pkt_count; i < n_packets; ++i) {
                char output_img_path_and_name[PATH_MAX];
                int written_len = snprintf(output_img_path_and_name, sizeof(output_img_path_and_name), "%s/%d.png", output_flow_img_folder, i);
                
                // Check if the output was truncated or if an error occurred
                if (written_len < 0 || written_len >= (int)sizeof(output_img_path_and_name)) {
                    fprintf(stderr, "Error: Output path is too long for the buffer. Skipping file.\n");
                    continue;
                }

                save_gray_image_png(output_img_path_and_name, row, image);
            }

            if (deal_pkt++ % 10 == 0) printf("Process %d flows\r", deal_pkt - 1);
        }
    }

    closedir(input_dir);
    printf("Processing complete. Total process %d flows\n", deal_pkt);
    return 0;
}

int HAST_Four(const char *pcap_folder, const char *output_img_folder,\
    const char *n_packets_str, const char *n_bytes_str, const char *opt) {
    printf("%s\n", opt);
    int n_packets = atoi(n_packets_str);
    int n_bytes = atoi(n_bytes_str);

    if (n_packets <= 0 || n_bytes <= 0) {
        fprintf(stderr, "Invalid number of packets or bytes: %s, %s\n", n_packets_str, n_bytes_str);
        return 1;
    }

    if (n_packets % 3 != 0) {
        fprintf(stderr, "Invalid number of n_bytes, n_bytes mod 3 must equal 0: %s\n", n_packets_str);
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

    // 設定 row 的長度
    int row = 0;
    if (n_bytes % 16 != 0){
        row = n_bytes / 16 + 1;
    }
    else row = n_bytes / 16;

    struct dirent *entry;
    char source_filepath[PATH_MAX];
    int deal_pkt = 0; // 用來計算處理的 packet 數量
    while ((entry = readdir(input_dir)) != NULL) {
        if (strstr(entry->d_name, ".pcap")) {
            snprintf(source_filepath, sizeof(source_filepath), "%s/%s", pcap_folder, entry->d_name);
            const uint8_t *pkt_arr[n_packets]; /* 建立一個 '存放 uint_8 指標' 的陣列 */
            int pkt_lens[n_packets];
            int pkt_count = extract_n_packets_from_pcap(source_filepath, n_packets, pkt_arr, pkt_lens);

            /* 替每一個 flow 建立一個新的資料夾 */
            char output_flow_img_folder[PATH_MAX];
            snprintf(output_flow_img_folder, sizeof(output_flow_img_folder), "%s/%s", output_img_folder, entry->d_name);
            if (create_directories(output_flow_img_folder) != 0) {
                fprintf(stderr, "Error: Could not create output directory '%s'.\n", output_flow_img_folder);
                continue; // 建立失敗，跳過這個 packet
            }
            
            uint8_t image[3][row][16];
            memset(image, 0, sizeof(unsigned char) * 3 * row * 16);
            // 將 i 設定為較大的全域變數的目的是讓後面 pkt_count % 3 != 0 的剩餘部份補 0 可以正確進行
            int i = 0;
            for (; i < pkt_count; ++i) {
                // 注意：這裡直接傳入 n_bytes，讓 get_n_byte_from_pkt 內部處理長度與填充
                uint8_t *n_byte_data = get_n_byte_from_pkt(n_bytes, pkt_arr[i], pkt_lens[i]);
                if (!n_byte_data) continue; // 如果記憶體分配失敗則跳過

                if (strcmp(opt, "de_addr_noise") == 0) {
                    __DENOISE_bytes_Grayscale_image(n_byte_data, n_bytes, row, image[i % 3],
                    pkt_lens[i]); // 使用去除 IP 的版本
                }
                else {
                    if (pkt_lens[i] > n_bytes){
                        // 直接將 n Bytes 先複製到 image 中
                        if(memcpy(image[i % 3], n_byte_data, n_bytes) != image[i % 3]) perror("Can't Create a new image.");
                    }

                    /*
                     * 當 pcap 抓取到的 bytes 小於實際所需時，僅會複製到實際所需的大小
                     */
                    else{
                        // 將實際上得到的 Bytes 數複製到 image 中即可
                        if(memcpy(image[i % 3], n_byte_data, pkt_lens[i]) != image[i % 3]) {
                            perror("Can't Create a new image.");
                        }
                    }
                }

                // 輸出圖片
                if (i % 3 == 2){
                    char output_img_path_and_name[PATH_MAX];

                    int written_len = snprintf(output_img_path_and_name, sizeof(output_img_path_and_name), "%s/%d.png", output_flow_img_folder, i / 3);
                    
                    // Check if the output was truncated or if an error occurred
                    if (written_len < 0 || written_len >= (int)sizeof(output_img_path_and_name)) {
                        fprintf(stderr, "Error: Output path is too long for the buffer. Skipping file.\n");
                        // Clean up and continue to the next iteration
                        free(n_byte_data);
                        free((void*)pkt_arr[i]);
                        continue; 
                    }
                
                    #ifdef DEBUG
                    uint8_t* data = image;
                
                    for(int i = 0; i < 16 * row; i++){
                        printf("%.2x ", data[i]);
                        if (i % 8 == 7) printf(" ");
                        if (i % 16 == 15) printf("\n");
                    }
                    printf("\n");
                    #endif
                
                    save_color_image_png(output_img_path_and_name, row, image);
                
                    free(n_byte_data);
                    free((void*)pkt_arr[i]);
                    memset(image, 0, sizeof(image));
                }
            }

            /* 空圖片 (資料準備) */
            uint8_t black_channel_image[row][16];
            memset(black_channel_image, 0, sizeof(uint8_t) * row * 16); // 清空圖片

            // 不齊未滿 3 通道的圖片
            while(i % 3 != 0){
                memcpy(image[i % 3], black_channel_image, sizeof(uint8_t) * row * 16);

                if(i % 3 == 2){
                    char output_img_path_and_name[PATH_MAX];
                    int written_len = snprintf(output_img_path_and_name, sizeof(output_img_path_and_name), "%s/%d.png", output_flow_img_folder, i / 3);
                    
                    // Check if the output was truncated or if an error occurred
                    if (written_len < 0 || written_len >= (int)sizeof(output_img_path_and_name)) {
                        fprintf(stderr, "Error: Output path is too long for the buffer. Skipping file.\n");
                        continue;
                    }
                    save_color_image_png(output_img_path_and_name, row, image);
                }
                i++;
            }

            uint8_t* black_image = malloc(sizeof(uint8_t) * row * 3 * n_bytes);
            memset(black_image, 0, sizeof(uint8_t) * row * 3 * n_bytes);
            // 補齊剩餘的空圖片
            for (; i < n_packets; i += 3){
                char output_img_path_and_name[PATH_MAX];
                int written_len = snprintf(output_img_path_and_name, sizeof(output_img_path_and_name), "%s/%d.png", output_flow_img_folder, i / 3);
                
                // Check if the output was truncated or if an error occurred
                if (written_len < 0 || written_len >= (int)sizeof(output_img_path_and_name)) {
                    fprintf(stderr, "Error: Output path is too long for the buffer. Skipping file.\n");
                    continue;
                }
                save_color_image_png(output_img_path_and_name, row, image);
            }
            free(black_image);

            if (deal_pkt++ % 10 == 0) printf("Process %d flows\r", deal_pkt - 1);
        }
    }

    closedir(input_dir);
    printf("Processing complete. Total process %d flows\n", deal_pkt);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 7) {
        help(argv[0]);
        return 1;
    }

    int status = 0;

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        help(argv[0]);
        return 0;
    }
    else if (strcmp(argv[1], "-f") == 0 || strcmp(argv[1], "--flow") == 0 || strcmp(argv[1], "-1") == 0 || strcmp(argv[1], "--hast-1" ) == 0 || strcmp(argv[1], "--HAST-I" ) == 0){
        if (argc > 6 || argc < 5) {
            printf("Usage: %s -f <pcap_folder> <output_img_folder> <n_bytes> \
                    <opt>\n", argv[0]);
            return 1;
        }

        if (argc == 5){
            status = HAST_ONE(argv[2], argv[3], argv[4], "NULL");
        }
        else if(argc == 6){
            status = HAST_ONE(argv[2], argv[3], argv[4], argv[5]);
        }

        if (status != 0) return status;
        return 0;
    }
    else if (strcmp(argv[1], "-p") == 0 || strcmp(argv[1], "--packet") == 0 || strcmp(argv[1], "-2") == 0 || strcmp(argv[1], "--hast-2" ) == 0 || strcmp(argv[1], "--HAST-II" ) == 0){
        if (argc > 7 || argc < 6) {
            printf("Usage: %s -p <pcap_folder> <output_img_folder> <n_packets> \
                    <n_byte> <opt>\n", argv[0]);
            return 1;
        }
        if (argc == 6) {
            status = HAST_TWO(argv[2], argv[3], argv[4], argv[5], "NULL");
        }
        else if (argc == 7) {
            status = HAST_TWO(argv[2], argv[3], argv[4], argv[5], argv[6]);
        }

        if (status != 0)return status;
        return 0;
    }
    else if (strcmp(argv[1], "-3") == 0){
        if (argc > 7 || argc < 6) {
            printf("Usage: %s -3 <pcap_folder> <output_img_folder> <n_packets> \
                    <n_byte> <opt>\n", argv[0]);
            return 1;
        }
        if (argc == 6) {
            status = HAST_Three(argv[2], argv[3], argv[4], argv[5], "NULL");
        }
        else if (argc == 7) {
            status = HAST_Three(argv[2], argv[3], argv[4], argv[5], argv[6]);
        }

        if (status != 0)return status;
        return 0;
    }
    else if (strcmp(argv[1], "-4") == 0){
        if (argc > 7 || argc < 6) {
            printf("Usage: %s -4 <pcap_folder> <output_img_folder> <n_packets> \
                    <n_byte> <opt>\n", argv[0]);
            return 1;
        }
        if (argc == 6) {
            status = HAST_Four(argv[2], argv[3], argv[4], argv[5], "NULL");
        }
        else if (argc == 7) {
            status = HAST_Four(argv[2], argv[3], argv[4], argv[5], argv[6]);
        }

        if (status != 0)return status;
        return 0;
    }

    help(argv[0]);
    return 1;
}
