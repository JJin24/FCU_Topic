#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include "common.h"

/*
 * 任務結構：
 * arg 為執行的函數
 * argument 為傳遞給函數的參數
 */
typedef struct {
    void (*function)(void *arg);
    void *argument;
} Task;

/*
 * 執行緒池結構 (尚未執行的 Task 會放在這裡)：
 * pthread_t threads: 用於紀錄已建立的執行緒 ID 陣列，內部單位為一個 pthread_t
 * Task queue: thread 尚未執行的工作陣列，內部單位 Task (代辦事項)
 * int thread_count: 目前在 Thread Pool 的執行緒數量
 * int queue_size: Thread Pool 最大可用來存放在的數量
 * int head: 目前在 queue 中尚未執行的 Task 的位置
 * int tail: 目前在 queue 中可以加入新尚未執行 Task 的位置
 * int count: 目前在 Thread Pool 所記錄未執行任務數量
 * pthread_mutex_t lock: 用來保護 ThreadPool 的互斥鎖 (避免多個執行緒同時修改)
 * pthread_cound_t notify: 用來通知執行緒有新的任務加入
 * int shutdown: 表示執行緒池是否關閉，當設定為 1 時，表示執行緒池開始執行關閉執行去緒的工作
 * 
 * 補充說明：ThreadPool 的儲存的設計會讓 head 和 tail 不斷的往上加 1，
 * 當 head 和 tail 超過 queue_size 時，會回到 0。(循環佇列)
 */
typedef struct {
    pthread_t *threads;
    Task *queue;
    int thread_count;
    int queue_size;
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t notify;
    int shutdown;
} ThreadPool;

ThreadPool *thread_pool_create(int thread_count, int queue_size);
int thread_pool_add_task(ThreadPool *pool, void (*function)(void *), void *argument);
void thread_pool_destroy(ThreadPool *pool);

#endif // THREAD_POOL_H