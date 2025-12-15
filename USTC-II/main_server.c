#include "common.h"
#include "thread_pool.h"
#include "HAST_Two.h"
#include "Flow2img4.h"

// 函式宣告
static int create_and_bind_unix_socket();
static void handle_new_connection(int epoll_fd, int server_fd);
// *** MODIFIED: Added epoll_fd parameter ***
static void handle_client_data(int epoll_fd, client_info *ci, ThreadPool *pool);
int choose_mode();

/*
 * 建立 convert_mode 讓使用選擇要使用 HAST_Two 的模式或是 Flow2img_4 的轉換
 * convert_mode 初始值為 0。
 * HAST_Two 模式時，convert_mode 為 1。
 * Flow2img_4 模式時，convert_mode 為 4。
 */
static int convert_mode = 0;

int main() {
    // 選擇模式
    if(choose_mode()){
        exit(1);
    }

    // 1. 建立並綁定 Unix Domain Socket
    int server_fd = create_and_bind_unix_socket();
    if (server_fd < 0) {
        return 1;
    }

    /*
     * 2. 建立 epoll 實例
     * 目前在較新版本的 Linux 2.6.8 中，epoll_create(int size) 中的 size 參數已經不重要了。
     * 現在裡面的 size 對 Kernel 來說意義只有在 hint (提示) 的作用，但實際上 Kernel 會主動
     * 分配合適數量的 epoll_event 出來。
     * 因此，使用 epoll_create1(0) 來建立一個新的 epoll 實例。(不需要提示 size)
     */
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(server_fd);
        return 1;
    }

    // 3. 將 server socket 加入 epoll 監聽
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = (void*)(intptr_t)server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epoll_ctl: server_fd");
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    // 4. 建立執行緒池
    ThreadPool *pool = thread_pool_create(MAX_THREADS, 100); // 100 為任務佇列大小
    if (pool == NULL) {
        fprintf(stderr, "Failed to create thread pool.\n");
        close(server_fd);
        close(epoll_fd);
        return 1;
    }
    
    /*
     * 雖然已經使用 epoll_create1() 建立 epoll，可以使用的 event 數量是未知的，但如果
     * 宣告可使用的 event 數量過小，可能會導致 epoll_wait() 無法正確處理所有事件，
     * 導致系統無法正常運作。
     * 因此，這裡宣告一個較大的事件陣列。
     */
    struct epoll_event events[MAX_EVENTS];
    printf("Server listening on %s\n", SOCKET_PATH);

    // 5. 進入主事件迴圈 (有新的連線到 socket)
    while (1) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            if ((int)(intptr_t)events[i].data.ptr == server_fd) {
                handle_new_connection(epoll_fd, server_fd);
            } 
            else {
                client_info *ci = (client_info *)events[i].data.ptr;
                // *** MODIFIED: Pass epoll_fd to the handler ***
                handle_client_data(epoll_fd, ci, pool);
            }
        }
    }

    // 6. 清理資源
    thread_pool_destroy(pool);
    close(server_fd);
    close(epoll_fd);
    unlink(SOCKET_PATH);

    return 0;
}


static int create_and_bind_unix_socket() {
    struct sockaddr_un addr;
    
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    fcntl(fd, F_SETFL, O_NONBLOCK);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    unlink(SOCKET_PATH);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, SOMAXCONN) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }
    
    return fd;
}


static void handle_new_connection(int epoll_fd, int server_fd) {
    struct sockaddr_un remote_addr;
    socklen_t addr_len = sizeof(remote_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&remote_addr, &addr_len);
    if (client_fd < 0) {
        perror("accept");
        return;
    }

    /*
     * 設定 client_fd 為 non-blocking fd，所以當資料傳送進來後會先被放在 kernel 的 buffer 中
     * 接著就可以等待讀取下一筆的資料進來
     */
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl");
        close(client_fd);
        return;
    }

    client_info *ci = malloc(sizeof(client_info));
    if (!ci) {
        perror("malloc client_info");
        close(client_fd);
        return;
    }
    ci->fd = client_fd;
    ci->state = STATE_READ_HEADER;
    ci->to_read = sizeof(uint32_t);
    ci->received = 0;
    ci->buffer = malloc(ci->to_read);

    /*
     * 建立 ET (edge trigger) 的 epoll_event 結構，並將 client_fd 加入 epoll 監聽。
     *
     * 說明：ET 模式下，當有資料可讀時 (ci->fd 狀態改變時)，epoll 只會通知一次。
     * 所以當有資料進來的時候 (不管是讀取 header 還是 body)，該 epoll_event 都會被單獨觸發。
     * 所以當 client 傳送 Header 進來的時候，epoll 會先觸發一次，記錄成單一事件 (資料在 Kernel 中)。
     * 然後當 client 傳送 Body 進來的時候，epoll 會再次觸發一次。
     * 所以在 handle_client_data() 中，會先 read client_fd 的資料，
     * 然後根據目前的狀態 (Header 或 Body) 來處理資料。
     */
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = ci;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
        perror("epoll_ctl: client_fd");
        free(ci->buffer);
        free(ci);
        close(client_fd);
    } else {
        printf("New connection accepted: fd %d\n", client_fd);
    }
}


/*
 * 處理 client 傳送過來的資料
 * 
 * 先備知識：
 * 1. 在設定為 non-blocking 的情況下，當有資料可讀時，資料會先被放在 kernel 的 buffer 中。
 * 2. 在 epoll 的 ET 模式下，當有資料可讀時 (ci->fd 狀態改變時)，epoll 只會通知一次。
 *    所以讀取 Header 和 Body 的時候，會分別觸發兩次事件。
 * 
 * 處理方式：
 * 因為 epoll 是設定為 EPOLLET 的模式，所以當 socket 有資料到達，他都會被 epoll 紀錄起來
 * ，資料會被放在 kernel buffer 中，當處理到該筆 epoll_event 的時候，他就會從 kernel 讀取相對應資料
 * 接著根據目前的狀態 (Header 或 Body) 來處理資料。
 */
static void handle_client_data(int epoll_fd, client_info *ci, ThreadPool *pool) {
    ssize_t count;
    
    // 讀取資料到 buffer 中 (header 和 body 兩者是單一事件 epoll 為 ET Mode，請見 Line 151 的說明)
    while (1) {
        count = read(ci->fd, ci->buffer + ci->received, ci->to_read - ci->received);

        // 錯誤
        if (count == -1){
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            perror("Read socket error:");
            goto close_conn;
        }

        // 對方連線退出
        if (count == 0){
            fprintf(stdout, "%d close connect", ci->fd);
            goto close_conn;
        }

        ci->received += count;

        // 根據目前的狀態處理資料
        if (ci->received == ci->to_read) {
            if (ci->state == STATE_READ_HEADER) {
                uint32_t body_size = ntohl(*(uint32_t*)ci->buffer);
                printf("fd %d: Header received, body size = %u\n", ci->fd, body_size);

                if (body_size != sizeof(Flow2img_II_context)) {
                     fprintf(stderr, "fd %d: Invalid body size %u. Expected %zu\n", ci->fd, body_size, sizeof(Flow2img_II_context));
                     goto close_conn;
                }

                ci->state = STATE_READ_BODY;
                ci->to_read = body_size;
                ci->received = 0;
                free(ci->buffer);
                ci->buffer = malloc(body_size);
                if (!ci->buffer) {
                    perror("malloc for body buffer");
                    goto close_conn;
                }

            } else if (ci->state == STATE_READ_BODY) {
                printf("fd %d: Body received, processing...\n", ci->fd);

                Flow2img_II_context *task_arg = malloc(sizeof(Flow2img_II_context));
                if (!task_arg) {
                     perror("malloc for task argument");
                     goto close_conn;
                }
                memcpy(task_arg, ci->buffer, sizeof(Flow2img_II_context));

                /*
                 * 將大端序的資訊轉換為小端序
                 * 
                 * 注意：如果要轉換 buffer 內的內容時，建議先複製一份出來，在進行轉換
                 * 避免修改的過程因為 struct 的對齊機制導致資料錯亂。
                 * 
                 * 需要大小端序轉換的資訊有：
                 * 除了 uint8_t 和 char 類型及其陣列類型的資料外，
                 * 其他都需要轉換大小端序。
                 * 例如：int[32] (內部的每一個元素都要單獨轉換)，uint32_t，long int 等
                 */
                task_arg->pcap_size = ntohl(task_arg->pcap_size);
                task_arg->s_port = ntohs(task_arg->s_port);
                task_arg->d_port = ntohs(task_arg->d_port);

                // 依據不同的 convert_mode 選不同的切換模式
                if(convert_mode == 1){
                    if (thread_pool_add_task(pool, Flow2img_HAST_Two, task_arg) != 0) {
                        fprintf(stderr, "Failed to add task to thread pool.\n");
                        free(task_arg);
                    }
                }
                else if(convert_mode == 4){
                    if (thread_pool_add_task(pool, Flow2img_4, task_arg) != 0) {
                        fprintf(stderr, "Failed to add task to thread pool.\n");
                        free(task_arg);
                    }
                }
                

                ci->state = STATE_READ_HEADER;
                ci->to_read = sizeof(uint32_t);
                ci->received = 0;
                free(ci->buffer);
                ci->buffer = malloc(ci->to_read);
            }
        }
    }

    
    return;

close_conn:
    printf("Closing connection: fd %d\n", ci->fd);
    // *** MODIFIED: Use the correct epoll_fd and remove the client fd ***
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ci->fd, NULL);
    close(ci->fd);
    free(ci->buffer);
    free(ci);
}

int choose_mode(){
    while(convert_mode != 1 && convert_mode != 4){
        if(convert_mode){
            printf("\n無效輸入！\n");
        }
        printf("- HAST_Two 模式請輸入 1\n- Flow2img_4 模式請輸入 4\n- 輸入 0 退出程式\n\n  請輸入選擇模式：");
        if(scanf("%d", &convert_mode) != 1){
            convert_mode = 99;
            // 清空輸入緩衝區中殘留的字元 (例如 'd' 和 '\n')
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
        if(!convert_mode){
            return 1;
        }
    }

    return 0;
}