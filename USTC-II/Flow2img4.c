#include "Flow2img4.h"
#include "redis_handler.h"

/* 外部 Header */
#include "de_addr_noise.h"
#include "HAST_Two.h"

// 主處理函式，將作為執行緒池的任務
void Flow2img_4(void *arg) {
    Flow2img_II_context *context = (Flow2img_II_context *)arg;
    if (context == NULL) return;

    if (N_PKTS <= 0 || M_BYTES <= 0) {
        fprintf(stderr, "Invalid N_PKTS or M_BYTES: %d, %d\n", N_PKTS, M_BYTES);
        free(context);
        return;
    }

    if (N_PKTS % 3 != 0) {
        fprintf(stderr, "Invalid number of N_PKTS mod 3 must equal 0: %d\n", N_PKTS);
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
    snprintf(metadata, sizeof(metadata), "{\"s_ip\":\"%s\",\"d_ip\":\"%s\",\"s_port\":\"%d\",\"d_port\":\"%d\",\"protocol\":\"%d\",\"timestamp\":\"%s\"}", 
             context->s_ip, context->d_ip, context->s_port, context->d_port, context->protocol, context->timestamp);
    reply = redisCommand(redis_conn, "HSET %s metadata %s", key, metadata);
    if (reply) freeReplyObject(reply);

    int row = 0;
    if (M_BYTES % 16 != 0){
        row = M_BYTES / 16 + 1;
    }
    else row = M_BYTES / 16;

    uint8_t image[3][row][16];
    memset(image, 0, sizeof(unsigned char) * 3 * row * 16);

    // 6. 處理每個封包，產生圖片並存入 Redis
    for(int i = 0; i < pkt_count; i++){
        // 注意：這裡直接傳入 n_bytes，讓 get_n_byte_from_pkt 內部處理長度與填充
        uint8_t *m_byte_data = get_n_byte_from_pkt(M_BYTES, pkt_arr[i], pkt_lens[i]);
        if (!m_byte_data) continue; // 如果記憶體分配失敗則跳過

        if (strcmp(OPT, "de_addr_noise") == 0) {
            __DENOISE_bytes_Grayscale_image(m_byte_data, M_BYTES, row, image[i % 3],
            pkt_lens[i]); // 使用去除 IP 的版本
        }
        else {
            if (pkt_lens[i] > M_BYTES){
                // 直接將 n Bytes 先複製到 image 中
                if(memcpy(image[i % 3], m_byte_data, M_BYTES) != image[i % 3]) perror("Can't Create a new image.");
            }

            /*
             * 當 pcap 抓取到的 bytes 小於實際所需時，僅會複製到實際所需的大小
             */
            else{
                // 將實際上得到的 Bytes 數複製到 image 中即可
                if(memcpy(image[i % 3], m_byte_data, pkt_lens[i]) != image[i % 3]) {
                    perror("Can't Create a new image.");
                }
            }
        }

        // 輸出圖片
        if (i % 3 == 2){

            // 將圖片轉換成 PNG 格式的記憶體區塊
            int png_size;

            // 將圖片的數據從平面式改成交錯式儲存
            uint8_t image_interleaved[row][16][3];

            for(int i = 0; i < row; i++){
                for(int j = 0; j < 16; j++){
                    image_interleaved[i][j][0] = image[0][i][j]; // R
                    image_interleaved[i][j][1] = image[1][i][j]; // G
                    image_interleaved[i][j][2] = image[2][i][j]; // B
                }
            }

            uint8_t *png_data = stbi_write_png_to_mem((const unsigned char *)image_interleaved, 0, 16, row, 3, &png_size);
            if (png_data) {
                snprintf(img_filename, sizeof(img_filename), "%d.png", i / 3);
                reply = redisCommand(redis_conn, "HSET %s %s %b", key, img_filename, png_data, (size_t)png_size);
                if (reply) freeReplyObject(reply);
                free(png_data);
            }
        
            free(m_byte_data);
            free((void*)pkt_arr[i]);
        }
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
