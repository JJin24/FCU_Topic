#include "HAST_Two.h"
#include "redis_handler.h"

/* 外部 Header */
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "de_addr_noise.h"

// 函式宣告 (因為它們只在此檔案中使用，設為 static)
static unsigned int extract_n_packets_from_pcap(uint8_t *pcap_data, size_t pcap_len, int n, const uint8_t **pkt_arr, int *pkt_lens);
static uint8_t* get_n_byte_from_pkt(int n, const uint8_t *pkt, int pkt_len);
static void bytes_to_onehot_image(const uint8_t *bytes, int n, unsigned char image[256][n], int current_get_bytes);


// 主處理函式，將作為執行緒池的任務
void Flow2img_HAST_Two(void *arg) {
    Flow2img_II_context *context = (Flow2img_II_context *)arg;
    if (context == NULL) return;

    if (N_PKTS <= 0 || M_BYTES <= 0) {
        fprintf(stderr, "Invalid N_PKTS or M_BYTES: %d, %d\n", N_PKTS, M_BYTES);
        free(context);
        return;
    }
    
    // 1. 連線至 Redis
    redisContext *redis_conn = redis_connect(REDIS_HOST, REDIS_PORT);
    if (redis_conn == NULL) {
        free(context);
        return;
    }

    // 2. 從 pcap data 中提取封包
    const uint8_t *pkt_arr[N_PKTS];
    int pkt_lens[N_PKTS];
    int pkt_count = extract_n_packets_from_pcap(context->pcap_data, context->pcap_size, N_PKTS, pkt_arr, pkt_lens);
    
    // 3. 準備 Redis 使用的 key 和其他資料
    char key[PATH_MAX], img_filename[32], metadata[256];
    snprintf(key, sizeof(key), "%s_%s_%s_%d_%d", context->timestamp, context->s_ip, context->d_ip, context->s_port, context->d_port);
    
    /*
     * 4. 將該筆 pcap data 存到 Redis 的 key 中
     * 存放到 Redis 指令：HSET <key> <field 欄位> <data 資料>
     */
    redisReply *reply = redisCommand(redis_conn, "HSET %s pcap_data %b", key, context->pcap_data, context->pcap_size);
    if (reply) freeReplyObject(reply);
    
    // 5. 將該筆 pcap metadata 存到 Redis 的 key 中
    snprintf(metadata, sizeof(metadata), "{\"s_ip\":\"%s\",\"d_ip\":\"%s\",\"s_port\":\"%d\",\"d_port\":\"%d\",\"protocol\":\"%d\"}", 
             context->s_ip, context->d_ip, context->s_port, context->d_port, context->protocol);
    reply = redisCommand(redis_conn, "HSET %s metadata %s", key, metadata);
    if (reply) freeReplyObject(reply);

    // 6. 處理每個封包，產生圖片並存入 Redis
    for (int i = 0; i < pkt_count; ++i) {
        uint8_t *n_byte_data = get_n_byte_from_pkt(M_BYTES, pkt_arr[i], pkt_lens[i]);
        if (!n_byte_data) continue;

        unsigned char image[256][M_BYTES];
        
        if (strcmp(OPT, "de_addr_noise") == 0) {
            __DENOISE_bytes_to_onehot_image(n_byte_data, M_BYTES, image, pkt_lens[i]);
        } else {
            bytes_to_onehot_image(n_byte_data, M_BYTES, image, pkt_lens[i]);
        }
        
        // 將圖片轉換成 PNG 格式的記憶體區塊
        int png_size;
        uint8_t *png_data = stbi_write_png_to_mem((const unsigned char *)image, 0, M_BYTES, 256, 1, &png_size);
        if (png_data) {
            snprintf(img_filename, sizeof(img_filename), "%d.png", i);
            reply = redisCommand(redis_conn, "HSET %s %s %b", key, img_filename, png_data, (size_t)png_size);
            if (reply) freeReplyObject(reply);
            free(png_data);
        }

        free(n_byte_data);
        free((void*)pkt_arr[i]);
    }

    // 7. 如果封包數不足 N_PKTS，用空圖片填充
    unsigned char empty_image[256][M_BYTES];
    memset(empty_image, 0, sizeof(unsigned char) * 256 * M_BYTES);
    int png_size;
    uint8_t *png_data = stbi_write_png_to_mem((const unsigned char *)empty_image, 0, M_BYTES, 256, 1, &png_size);

    if(png_data) {
        for (int i = pkt_count; i < N_PKTS; ++i) {
            snprintf(img_filename, sizeof(img_filename), "%d.png", i);
            reply = redisCommand(redis_conn, "HSET %s %s %b", key, img_filename, png_data, (size_t)png_size);
            if (reply) freeReplyObject(reply);
        }
        free(png_data);
    }
    
    // 6. 清理資源
    redis_disconnect(redis_conn);
    free(context); // 釋放傳遞過來的 context 記憶體
    printf("Task for key %s finished.\n", key);
}

/*
 * buffer 中擷取前 n 個 packet，回傳 packet 指標陣列及實際數量及每個 packet 的長度。
 * input:
 *   - pcap_data: 存有 pcap 資料的 buffer
 *   - pcap_len: pcap 資料的長度
 *   - n: 要擷取的 packet 數量
 *   - pkt_arr: 指向每個 packet 的指標陣列
 *   - pkt_lens: 每個 packet 的長度陣列
 * return value:
 *   - 回傳實際擷取的 pkt 數量
 *   - pkt_arr: 指向每個 packet 的指標陣列
 *   - pkt_lens: 每個 pkt 的長度陣列
 */
static unsigned int extract_n_packets_from_pcap(uint8_t *pcap_data, size_t pcap_len, int n, const uint8_t **pkt_arr, int *pkt_lens) {
    char errbuf[PCAP_ERRBUF_SIZE];
    // 使用 fmemopen 從記憶體中讀取 pcap data
    FILE *pcap_stream = fmemopen(pcap_data, pcap_len, "rb");
    if (!pcap_stream) {
        perror("fmemopen");
        return 0;
    }

    pcap_t *handle = pcap_fopen_offline(pcap_stream, errbuf);
    if (!handle) {
        fprintf(stderr, "pcap_fopen_offline(): %s\n", errbuf);
        fclose(pcap_stream);
        return 0;
    }

    struct pcap_pkthdr *header;
    const uint8_t *data;
    int count = 0;

    while (count < n && pcap_next_ex(handle, &header, &data) == 1) {
        pkt_arr[count] = malloc(header->caplen);
        if(pkt_arr[count] == NULL) {
             // 記憶體分配失敗，清理已分配的並返回
            for(int i = 0; i < count; i++) free((void*)pkt_arr[i]);
            count = 0;
            break;
        }
        memcpy((void*)pkt_arr[count], data, header->caplen);
        pkt_lens[count] = header->caplen;
        count++;
    }

    pcap_close(handle); // 這會自動 fclose(pcap_stream)
    return count;
}


static uint8_t* get_n_byte_from_pkt(int n, const uint8_t *pkt, int pkt_len) {
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
static void bytes_to_onehot_image(const uint8_t *bytes, int n, unsigned char image[256][n], int current_get_bytes) {
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
