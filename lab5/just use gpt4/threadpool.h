#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

// Node structure for queues
typedef struct node
{
    void *data;
    struct node *next;
} node_t;

// Queue structure
typedef struct
{
    node_t *head;
    node_t *tail;
    pthread_mutex_t lock;
} queue_t;

// Thread pool structure
typedef struct
{
    pthread_t *threads;
    queue_t *task_queue;
    int thread_count;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} threadpool_t;

// Function prototypes
threadpool_t *create_threadpool(int num_threads);
void threadpool_add(threadpool_t *pool, void *(*function)(void *), void *argument);
void *thread_worker(void *arg);

// Your specific data structures for filename_queue and msg_queue
typedef struct
{
    char *filepath;
    int socket;
} filename_task_t;

typedef struct
{
    char *file_content;
    int socket;
} msg_task_t;

#endif
