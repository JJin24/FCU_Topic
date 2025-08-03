#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>

// 從 Thread Pool 中取出代辦事項並執行
static void *thread_pool_worker(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    Task task;

    /*
     * thread 在建立的時候就會進入這一個 while loop
     * 他會不斷的尋找、執行、等待在 Thread Pool 所記錄新的工作
     * 在等待時不會消耗 CPU 的資源
     */
    while (1) {
        pthread_mutex_lock(&(pool->lock));

        /*
         * 等待佇列中有任務或執行緒池被關閉
         * 當 Thread Pool 中沒有任務時，執行緒會進入休眠模式，
         * 避免 CPU 不斷的在執行等待工作。
         * 當有新的工作 (pool->notify 不為 0) 時，會喚醒一個等待中的執行緒。
         * 
         * 並且在 pthread_cond_wait 中會自動的將 mutex 解鎖，
         * 當等待結束後，mutex 會自動被重新鎖定，所以不用特別寫 unlock 及 lock 該互斥鎖
         */
        while (pool->count == 0 && !pool->shutdown) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if (pool->shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        // 從佇列中取出任務
        task.function = pool->queue[pool->head].function;
        task.argument = pool->queue[pool->head].argument;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count--;

        pthread_mutex_unlock(&(pool->lock));

        // 執行任務
        (*(task.function))(task.argument);
    }

    return NULL;
}

ThreadPool *thread_pool_create(int thread_count, int queue_size) {
    ThreadPool *pool;

    if (thread_count <= 0 || queue_size <= 0) {
        return NULL;
    }

    if ((pool = (ThreadPool *)malloc(sizeof(ThreadPool))) == NULL) {
        perror("malloc(ThreadPool)");
        return NULL;
    }

    // 初始化執行緒池屬性
    pool->thread_count = 0;
    pool->queue_size = queue_size;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = 0;

    // 分配記憶體給執行緒和任務佇列
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    pool->queue = (Task *)malloc(sizeof(Task) * queue_size);

    if (pool->threads == NULL || pool->queue == NULL) {
        perror("malloc(threads or queue)");
        free(pool->threads);
        free(pool->queue);
        free(pool);
        return NULL;
    }
    
    // 初始化互斥鎖和條件變數
    pthread_mutex_init(&(pool->lock), NULL);
    pthread_cond_init(&(pool->notify), NULL);

    // 創建工作執行緒
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, thread_pool_worker, (void *)pool) != 0) {
            thread_pool_destroy(pool);
            return NULL;
        }
        pool->thread_count++;
    }

    return pool;
}

// 有新的任務 (代辦事項) 加入到 Thread Pool
int thread_pool_add_task(ThreadPool *pool, void (*function)(void *), void *argument) {
    if (pool == NULL || function == NULL) {
        return -1;
    }

    pthread_mutex_lock(&(pool->lock));

    if (pool->count == pool->queue_size) {
        // 佇列已滿
        pthread_mutex_unlock(&(pool->lock));
        fprintf(stderr, "Task queue is full.\n");
        return -1;
    }
    
    // 將任務加入佇列
    pool->queue[pool->tail].function = function;
    pool->queue[pool->tail].argument = argument;
    pool->tail = (pool->tail + 1) % pool->queue_size;
    pool->count++;
    
    // 通知一個等待中的工作執行緒
    pthread_cond_signal(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));

    return 0;
}

// thread pool 被破壞或是程式中斷時呼叫的函數，此程式將會關閉所有執行緒並釋放資源
void thread_pool_destroy(ThreadPool *pool) {
    if (pool == NULL) {
        return;
    }

    pthread_mutex_lock(&(pool->lock));
    pool->shutdown = 1;
    pthread_cond_broadcast(&(pool->notify)); // 喚醒所有執行緒以便它們結束
    pthread_mutex_unlock(&(pool->lock));

    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    
    free(pool->threads);
    free(pool->queue);
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));
    free(pool);
}