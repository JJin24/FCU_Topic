#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
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

/* Flow2img 相關定義 */
#define N_PKTS 6
#define M_BYTES 100
#define OPT "de_addr_noise"
#define MAX_PKT_LEN 2048

/* Server 相關定義 */
#define MAX_EVENTS 20
#define MAX_THREADS 8 // 可依據你的 CPU 核心數調整
#define SOCKET_PATH "/tmp/Flow2img_II_Proxy.sock"

/* Redis Server 相關定義 */
#define REDIS_HOST "127.0.0.1"
#define REDIS_PORT 6379

/* 
 * 讀取狀態機
 * STATE_READ_HEADER: 等待讀取資料長度 (4 bytes)
 * STATE_READ_BODY: 等待讀取資料本體
 */
typedef enum {
    STATE_READ_HEADER,
    STATE_READ_BODY
} ReadState;


/*
 * 抓包程式傳過來的 "資料結構"
 * 注意：Client 端傳送時也必須使用此結構
 */
typedef struct {
    uint8_t pcap_data[MAX_PKT_LEN * N_PKTS];
    uint32_t pcap_size;
    char s_ip[INET_ADDRSTRLEN];
    char d_ip[INET_ADDRSTRLEN];
    int s_port;
    int d_port;
    char timestamp[64];
} Flow2img_II_context;


/*
 * clientinfo 的用途在於追蹤每一個 client socket 的狀態
 * 1. fd: socket file descriptor
 * 2. state: 目前的讀取狀態 (讀取 header 或 body)
 * 3. buffer: 用來暫存從 socket 讀進來的資料
 * 4. to_read: 預期需要讀取的 bytes 數量 (需要在下次讀取前需要設定好了)
 * 5. received: 已經接收了多少 bytes
 */
typedef struct {
    int fd;
    ReadState state;
    uint8_t *buffer;
    uint32_t to_read;
    uint32_t received;
} client_info;


#endif // COMMON_H