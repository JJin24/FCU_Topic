/*
 * - The epoll_server from https://www.geekpage.jp/ author
 * - stb_image_write from https://github.com/nothings/stb
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <dirent.h>
#include <linux/limits.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

/*
 * 外部 Header
 * 額外注意的是下面這一個 define 要先寫，才可以使用 std_image_write.h 的內容
 * 這個原因是因為
 */
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "de_addr_noise.h"

/* Flow2img 相關定義 */
#define N_PKTS 6
#define M_BYTES 100
#define OPT "de_addr_noise"
#define MAX_PKT_LEN 2048
#define N_EVENTS 20
#define MYSTATE_READ 0
#define MYSTATE_WRITE 1
#define MAX_THREADS 4

/* Redis Server 相關定義 */
#define REDIS_HOST "127.0.0.1"
#define REDIS_PORT 6379

/*
 * clientinfo 的用途在於：
 * 1. 儲存 accept 進來的 fd
 * 2. 資料傳進來但 epoll_wait() 沒有處理完的時候
 *    ，傳進來的資料會必須先放在 buf 中 (每個 socket 單獨一個 buf)
 * 3. 紀錄 sock 內紀錄的長度
 * 4. state 用來判斷狀況 sock 寫完了沒有，如果沒有的話，就繼續的將資料寫完
 */
struct clientinfo {
    int fd;
    struct Flow2img_II_context *context;
    int sock_data_size;
    size_t now_arr_size;
    int state;
};

/*
 * 抓包程式傳過來的 "資料結構"，看有沒有需要也記錄一下 IPv6 的
 */
struct Flow2img_II_context {
    uint8_t pcap_data[MAX_PKT_LEN * N_PKTS];
    size_t pcap_size;
    char s_ip[INET_ADDRSTRLEN];
    char d_ip[INET_ADDRSTRLEN];
    int s_port;
    int d_port;
    char timestamp[64];
};

struct save_img_context {
    char filename[PATH_MAX];
    int cols;
    uint8_t image[256][PATH_MAX];
};

int active_threads = 0;
pthread_mutex_t count_mutex;
pthread_cond_t count_cond;

unsigned int extract_n_packets_from_pcap(uint8_t *pcap_data, int n, const uint8_t **pkt_arr, int *pkt_lens);
uint8_t* get_n_byte_from_pkt(int n, const uint8_t *pkt, int pkt_len);
void bytes_to_onehot_image(const uint8_t *bytes, int n, unsigned char image[256][n], int current_get_bytes);
void* HAST_TWO(void *arg);
void commandCallback(redisAsyncContext *c, void *r, void *privdata);

int main (){
    /*
     * epoll 相關變數：
     * - ep_event 是專門的 epoll 事件處理器
     * - ev 是專門設定 epoll 處理的事件，透過 epoll_ctl() 將 ev 的事件設定給 ep_event 做處理
     * - ev_ret 是用來接收 epoll_wait() 回傳的事件 (可能同時有 N_EVENTS 個事件，所以要建立那麼多個)
     */
    struct epoll_event ev, ev_ret[N_EVENTS];
    int ep_event = epoll_create(N_EVENTS);
    int Flow2img_II_Proxy = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;

    if (Flow2img_II_Proxy < 0) {
        perror("socket (Flow2img_II_Proxy)");
        return 1;
    }

    int sock_status = fcntl(Flow2img_II_Proxy, F_SETFL, O_NONBLOCK);
    if (sock_status < 0) {
        perror("fcntl (Flow2img_II_Proxy)");
        close(Flow2img_II_Proxy);
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "/tmp/Flow2img_II_Proxy.sock");
    if(bind(Flow2img_II_Proxy, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind (Flow2img_II_Proxy)");
        close(Flow2img_II_Proxy);
        return 1;
    }

    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.ptr = malloc(sizeof(struct clientinfo));
    if (ev.data.ptr == NULL) {
        perror("malloc");
        return 1;
    }
    memset(ev.data.ptr, 0, sizeof(struct clientinfo));
    ((struct clientinfo *)ev.data.ptr)->fd = Flow2img_II_Proxy;

    if (listen(Flow2img_II_Proxy, N_EVENTS) < 0){
        perror("listen (Flow2img_II_Proxy)");
        close(Flow2img_II_Proxy);
        return 1;
    }

    if(epoll_ctl(ep_event, EPOLL_CTL_ADD, Flow2img_II_Proxy, &ev) != 0) {
        perror("epoll_ctl (Flow2img_II_Proxy)");
        close(Flow2img_II_Proxy);
        return 1;
    }

    pthread_mutex_init(&count_mutex, NULL);
    pthread_cond_init(&count_cond, NULL);

    while(1){
        printf("Waiting for connections...\n");
        int nfds = epoll_wait(ep_event, ev_ret, N_EVENTS, -1);
        if (nfds < 0) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            struct clientinfo *ci = ev_ret[i].data.ptr;
            struct clientinfo client;

            /* 新的連線進入 */
            if (ci->fd == Flow2img_II_Proxy) {
                int Proxy_client = accept(Flow2img_II_Proxy, (struct sockaddr *)&client, &(socklen_t){sizeof(addr)});
                if (Proxy_client < 0) {
                    perror("accept (Flow2img_II_Proxy)");
                    return 1;
                }

                memset(&ev, 0, sizeof(ev));
                ev.events = EPOLLIN;
                ev.data.ptr = malloc(sizeof(struct clientinfo));
                if (ev.data.ptr == NULL) {
                    perror("malloc (clientinfo)");
                    return 1;
                }

                memset(ev.data.ptr, 0, sizeof(struct clientinfo));
                ((struct clientinfo *)ev.data.ptr)->fd = Proxy_client;

                if(epoll_ctl(ep_event, EPOLL_CTL_ADD, Proxy_client, &ev) != 0) {
                    perror("epoll_ctl (Proxy_client)");
                    return 1;
                }
            }

            /* 處理已經連線的客戶端 */
            else{
                /* 讀取資料 */
                ci->sock_data_size = read(ci->fd, ci->context, sizeof(ci->context));
                if (ci->sock_data_size < 0) {
                    perror("read");
                    return 1;
                }
                else if (ci->sock_data_size == 0) {
                    /* 客戶端關閉連線 */
                    epoll_ctl(ep_event, EPOLL_CTL_DEL, ci->fd, NULL);
                    close(ci->fd);
                    free(ci);
                    continue;
                }

                ci->state = MYSTATE_WRITE; // 假設 MYSTATE_WRITE 是一個已定義的狀態
                ev_ret[i].events = EPOLLOUT;

                if (epoll_ctl(ep_event, EPOLL_CTL_MOD, ci->fd, &ev_ret[i]) != 0) {
                    perror("epoll_ctl (EPOLLOUT)");
                    return 1;
                }

                /* 達到上限了，就等待 */
                pthread_mutex_lock(&count_mutex);
                while (active_threads >= MAX_THREADS) {
                    printf("Active threads limit reached (%d), waiting...\n", active_threads);
                    pthread_cond_wait(&count_cond, &count_mutex);
                }

                pthread_t tid;
                int pthread_status = pthread_create(&tid, NULL, HAST_TWO, ci->context);

                if (pthread_status != 0) {
                    perror("pthread_create");
                    pthread_mutex_unlock(&count_mutex);
                    return 1;
                }

                pthread_detach(tid);
                active_threads++;
                pthread_mutex_unlock(&count_mutex);
            }
        }
    }

    pthread_mutex_destroy(&count_mutex);
    pthread_cond_destroy(&count_cond);
}

void commandCallback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = r;
    char *request_desc = (char *)privdata;

    if (reply == NULL) {
        printf("Request '%s': reply is NULL. Context error: %s\n", request_desc, c->errstr);
        return;
    }
    
    freeReplyObject(reply);
}

/*
 * 從 pcap 檔案中擷取前 n 個 packet，回傳 packet 指標陣列及實際數量及每個 packet 的長度。
 * input:
 *   - pcap_data: pcap 檔案
 *   - n: 要擷取的 packet 數量
 *   - pkt_arr: 指向每個 packet 的指標陣列
 *   - pkt_lens: 每個 packet 的長度陣列
 * return value:
 *   - 回傳實際擷取的 pkt 數量
 *   - pkt_arr: 指向每個 packet 的指標陣列
 *   - pkt_lens: 每個 pkt 的長度陣列
 */
unsigned int extract_n_packets_from_pcap(uint8_t *pcap_data, int n, const uint8_t **pkt_arr, int *pkt_lens) {
    char errbuf[PCAP_ERRBUF_SIZE];
    FILE *pcap = fmemopen((void *)&pcap_data, sizeof(pcap_data), "rb");

    if (!pcap) {
        fprintf(stderr, "Couldn't open pcap data: %s\n", errbuf);
        return 0;
    }

    pcap_t *handle = pcap_fopen_offline(pcap, errbuf);

    if (!handle) {
        fprintf(stderr, "Couldn't open pcap file : %s\n", errbuf);
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

void* HAST_TWO(void *arg) {
    struct Flow2img_II_context *context = (struct Flow2img_II_context *)arg;
    if (N_PKTS <= 0 || M_BYTES <= 0) {
        fprintf(stderr, "Invalid number of packets or bytes: %d, %d\n", N_PKTS, M_BYTES);
        pthread_exit(NULL);
    }

    if (N_PKTS >= 4095){
        fprintf(stderr, "Warning: n_packets is too large, it may cause memory issues.\n");
        pthread_exit(NULL); // 如果 n_packets 太大，直接結束程式
    }

    redisAsyncContext *redis_server = NULL;
    redis_server = redisAsyncConnect(REDIS_HOST, REDIS_PORT);
    if (redis_server == NULL || redis_server->err) {
        fprintf(stderr, "Error: %s\n", redis_server ? redis_server->errstr : "Can't allocate redis context");
        pthread_exit(NULL);
    }

    const uint8_t *pkt_arr[N_PKTS]; /* 建立一個 '存放 uint_8 指標' 的陣列 */
    int pkt_lens[N_PKTS];
    int pkt_count = extract_n_packets_from_pcap(context->pcap_data, N_PKTS, pkt_arr, pkt_lens);

    /* 準備 Redis 使用的 key */
    char key[PATH_MAX], img_filename[16];
    snprintf(key, sizeof(key), "%s_%s_%s_%d_%d", context->timestamp, context->s_ip, context->d_ip, context->s_port, context->d_port);

    redisAsyncCommand(redis_server, commandCallback, "HSET %s %s %b", key, "pcap_data", context->pcap_data, context->pcap_size);

    char metadata[256];
    snprintf(metadata, sizeof(metadata), "\"s_ip\":\"%s\",\"d_ip\":\"%s\",\"s_port\":\"%d\",\"d_port\":\"%d\"", 
             context->s_ip, context->d_ip, context->s_port, context->d_port);
    redisAsyncCommand(redis_server, commandCallback, "HSET %s metadata %s", key, metadata);

    for (int i = 0; i < pkt_count; ++i) {
        // 注意：這裡直接傳入 n_bytes，讓 get_n_byte_from_pkt 內部處理長度與填充
        uint8_t *n_byte_data = get_n_byte_from_pkt(M_BYTES, pkt_arr[i], pkt_lens[i]);
        if (!n_byte_data) continue; // 如果記憶體分配失敗則跳過

        unsigned char image[256][M_BYTES];

        if (strcmp(OPT, "de_addr_noise") == 0) {
            __DENOISE_bytes_to_onehot_image(n_byte_data, M_BYTES, image,
            pkt_lens[i]); // 使用去除 IP 的版本
        }
        else {
            bytes_to_onehot_image(n_byte_data, M_BYTES, image, pkt_lens[i]);
        }

        uint8_t *img = stbi_write_png_to_mem((const unsigned char *)image, M_BYTES * sizeof(uint8_t), M_BYTES, 256, 1, NULL);
    
        snprintf(img_filename, sizeof(img_filename), "%d.png", i);
        redisAsyncCommand(redis_server, commandCallback, "HSET %s %s %b", key, img_filename, img, sizeof(*img));
        free(n_byte_data);
        free((void*)pkt_arr[i]);
    }

    /* 空圖片 (資料準備) */
    unsigned char image[256][M_BYTES];
    memset(image, 0, sizeof(unsigned char) * 256 * M_BYTES); // 清空圖片

    /* 填充空圖片 */
    for (int i = pkt_count; i < N_PKTS; ++i) {
        uint8_t *img = stbi_write_png_to_mem((const unsigned char *)image, M_BYTES * sizeof(uint8_t), M_BYTES, 256, 1, NULL);

        snprintf(img_filename, sizeof(img_filename), "%d.png", i);
        redisAsyncCommand(redis_server, commandCallback, "HSET %s %s %b", key, img_filename, img, sizeof(*img));
    }

    redisAsyncDisconnect(redis_server);
    /* 解除互斥鎖，退出 pthread */
    pthread_mutex_lock(&count_mutex);
    active_threads--;
    pthread_cond_signal(&count_cond);
    pthread_mutex_unlock(&count_mutex);
    pthread_exit(NULL);
}