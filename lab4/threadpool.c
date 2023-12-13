/*queue status and conditional variablex*/
void push_taskquene(taskqueue *queue, task *curtask);
task *take_taskqueue(taskqueue *queue);
void init_taskqueue(taskqueue *queue);
void destory_taskqueue(taskqueue *queue);

typedef struct staconv
{
    pthread_mutex_t tmutex;
    pthread_cond_t cond; /*用于阻塞和唤醒线程池中的线程*/
    int status;          /*表示任务队列状态:false 表示无任务;true表示有任务*/
} staconv;

/*Task*/
typedef struct task
{
    struct task *next;           /*指向下一个任务*/
    void (*function)(void *arg); /*函数指针*/
    void *arg;
} task; /*函数参数指针*/

/*Task Queue*/
typedef struct taskqueue
{
    pthread_mutex_t mutex; /*用于互斥读/写任务队列*/
    task *front;           /*指向队首*/
    task *rear;            /*指向队尾*/
    staconv *has_jobs;     /*根据状态，阻塞线程*/
    int len;               /*队列中任务的个数*/
} taskqueue;

/*Thread*/
typedef struct thread
{
    int id;                  /*线程id*/
    pthread_t pthread;       /*封装的POSIX线程*/
    struct threadpool *pool; /*与线程池绑定*/
} thread;

/*Thread Pool*/
typedef struct threadpool
{
    thread **threads;                /* 线程指针数组 */
    volatile int num_threads;        /* 线程池中线程数量 */
    volatile int num_working;        /* 目前正在工作的线程数量 */
    pthread_mutex_t thcount_lock;    /* 线程池锁用于修改上面两个变量 */
    pthread_cond_t threads_all_idle; /* 用于销毁线程的条件变量 */
    taskqueue queue;                 /* 任务队列 */
    volatile bool is_alive;          /* 表示线程池是否还存在 */
} threadpool;                        /* 线程池初始化函数 */

struct threadpool *initThreadPool(int num_threads)
{
    // 创建线程池空间
    threadpool *pool;
    pool = (threadpool *)malloc(sizeof(struct threadpool));
    pool->num_threads = 0;
    pool->num_working = 0;

    // 初始化互斥量和条件变量
    pthread_mutex_init(&(pool->thcount_lock), NULL);
    pthread_cond_init(&(pool->threads_all_idle), NULL);

    // 初始化任务队列
    // 需要实现initTaskQueue函数
    init_taskqueue(&(pool->queue));

    // 创建线程数组
    pool->threads = (struct thread **)malloc(num_threads * sizeof(struct * thread));

    // 创建线程
    for (int i = 0; i < num_threads; ++i)
    {
        create_thread(pool, pool->threads[i], i); // 为线程分配id
    }
    // 待所有的线程创建完毕，在每个线程运行函数中将进行 pool->num_threads++ 操作
    // 因此，此处为忙等待，直到所有的线程创建完毕，并在马上运行阻塞代码时才返回
    while (pool->num_threads != num_threads)
    {
    }
    return pool;
}

/*向线程池中添加任务*/
void addTask2ThreadPoo1(threadpool *pool, task *curtask)
{
    // 将任务加入队列
    // 需实现push taskqueue函数
    push_taskquene(&pool->queue, curtask);
}

/*等待当前任务全部运行完毕*/
void waitThreadPool(threadpool *pool)
{
    pthread_mutex_lock(&pool->thcount_lock);
    while (pool->queue.len || pool->num_working)
    {
        pthread_cond_wait(&pool->threads_all_idle & pool->thcount_lock);
    }
    pthread_mutex_unlock(&pool->thcount_lock);
}

/*销毁线程池*/
void destoryThreadPool(threadpool *pool)
{
    // 如果当前任务队列中有任务，则需等待任务队列为空，并且运行线程以执行完任务
    waitThreadPool(pool);

    // 销毁任务队列
    destory_taskqueue(&(pool->queue));

    // 销毁线程指针数组，并释放所有为线程池分配的内存
    for (int i = 0; i < pool->num_threads; ++i)
    {
        free(pool->threads[i]);
    }
    free(pool->threads);

    // 销毁互斥量和条件变量
    pthread_mutex_destroy(&(pool->thcount_lock));
    pthread_cond_destroy(&(pool->threads_all_idle));

    // 释放线程池内存
    free(pool);
}

/*获得当前线程池中正在运行线程的数量*/
int getNumofThreadWorking(threadpool *pool)
{
    return pool->num_working;
}

/*创建线程*/
int create_thread(struct threadpool *pool, struct thread **pthread, int id)
{
    // 为thread分配内存空间
    *pthread = (struct thread *)malloc(sizeof(struct thread));
    if (pthread == NULL)
    {
        error("creat thread(): Could not allocate memory for thread\n");
        return -1;
    }
    // 设置该thread 的属性(*pthread)->pool = pool;
    (*pthread)->pool = pool;
    (*pthread)->id = id;
    // 创建线程
    pthread_create(&(*pthread)->pthread, NULL, (void *)thread_do, (*pthread));
    pthread_detach((*pthread)->pthread);
    return 0;
}

/*线程运行的逻辑函数*/
void *thread_do(struct thread *pthread)
{
    /*设置线程的名称 */
    char thread_name[128] = {0};
    sprintf(thread_name, "thread-pool-%d", pthread->id);
    prctl(PR_SET_NAME, thread_name);
    /*获得线程池*/
    threadpool *pool = pthread->pool;
    /*在初始化线程池时，对已经创建线程的数量进行统计*/
    pool->num_threads++;
    /*线程一直循环运行，直到pool->is alive 变为false*/
    while (pool->is_alive)
    {
        /*如果任务队列中还有任务，则继续运行;否则阻塞*/

        if (pool->is_alive)
        {
            /*执行到此位置，表明线程在工作，需要对工作线程数量进行统计*/
            pool->num_working++;
            /*从任务队列的队首提取任务，并执行该任务*/
            void (*func)(void *);
            void *arg;
            // take_taskqueue 用于从任务队列的队首提取任务，并在队列中删除此任务
            task *curtask = take_taskqueue(&pool->queue);
            if (curtask)
            {
                func = curtask->function;
                arg = curtask->arg;
                // 执行任务
                func(arg);
                // 释放任务
                free(curtask);
            }

            /*执行到此位置，表明线程已经将任务执行完毕，需改变工作线程数量*/
            /*从此处还需注意，当工作线程数量为0时，表示任务全部完成，会使阻塞在waitThreadPool函数上的线程继续运行*/
        }
    }
    /*运行到此位置表明线程将要退出，需改变当前线程池中的线程数量*/
    pool->num_threads--;
    return NULL;
}
void push_taskquene(taskqueue *queue, task *curtask)
{
    pthread_mutex_lock(&queue->mutex);
    if (queue->len == 0)
    {
        queue->front = curtask;
        queue->rear = curtask;
    }
    else
    {
        queue->rear->next = curtask;
        queue->rear = curtask;
    }
    queue->len++;
    pthread_mutex_unlock(&queue->mutex);
}
task *take_taskqueue(taskqueue *queue)
{
    pthread_mutex_lock(&queue->mutex);
    task *curtask;
    if (queue->len == 0)
    {
        curtask = NULL;
    }
    else
    {
        curtask = queue->front;
        queue->front = curtask->next;
        queue->len--;
    }
    pthread_mutex_unlock(&queue->mutex);
    return curtask;
}
void init_taskqueue(taskqueue *queue)
{
    queue->front = NULL;
    queue->rear = NULL;
    queue->len = 0;
    pthread_mutex_init(&queue->mutex, NULL);
}
void destory_taskqueue(taskqueue *queue)
{
    pthread_mutex_destroy(&queue->mutex);
}