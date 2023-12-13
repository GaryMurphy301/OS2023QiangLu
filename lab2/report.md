###### 题目1：使用fork函数，设计并实现WebServer以支持多进程并发处理众多客户端的请求。

要使用`fork`函数支持多进程并发处理客户端请求，可以在`main`函数中创建一个循环，为每个传入的连接fork一个新进程。以下是修改后的代码：

```c
#include <sys/wait.h>
// ... (其余的代码)
int main(int argc, char **argv)
{
  // ... (现有的代码)
  for (hit = 1;; hit++)
  {
    length = sizeof(cli_addr);
    if ((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
      logger(ERROR, "system call", "accept", 0);
    // Fork一个新的进程
    if ((pid = fork()) < 0)
    {
      logger(ERROR, "system call", "fork", 0);
    }
    else if (pid == 0) // 子进程
    {
      close(listenfd); // 子进程不需要监听器
      // 处理请求
      web(socketfd, hit);
      exit(0); // 子进程处理完请求后退出
    }
    else // 父进程
    {
      close(socketfd); // 父进程不需要已接受的套接字
    }
  }
}

```



###### 题目2：使用信号量、共享内存等系统接口函数，来统计每个子进程的消耗时间以及所有子进程消耗时间之和。

在下面这个修改后的代码中，每个子进程在处理完请求后，会记录开始和结束时间，并将消耗时间加入到共享内存中，使用POSIX信号量保护共享内存的写操作，以防止多个子进程同时写入。同时，父进程在最后也会统计所有子进程的总消耗时间。

```c
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>

// 全局变量
sem_t *semaphore;
long long *total_time; // 共享内存

// ... (其余的代码)

int main(int argc, char **argv)
{
  // ... (现有的代码)

  // 初始化POSIX信号量
  semaphore = sem_open("/my_semaphore", O_CREAT | O_EXCL, 0644, 1);
  if (semaphore == SEM_FAILED)
  {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }

  // 初始化共享内存
  int shm_fd = shm_open("/my_shared_memory", O_CREAT | O_RDWR, 0644);
  if (shm_fd == -1)
  {
    perror("shm_open");
    exit(EXIT_FAILURE);
  }
  ftruncate(shm_fd, sizeof(long long));
  total_time = (long long *)mmap(NULL, sizeof(long long), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (total_time == MAP_FAILED)
  {
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  // ... (现有的代码)

  for (hit = 1;; hit++)
  {
    // ... (现有的代码)

    // Fork一个新的进程
    if ((pid = fork()) < 0)
    {
      logger(ERROR, "system call", "fork", 0);
    }
    else if (pid == 0) // 子进程
    {
      close(listenfd); // 子进程不需要监听器

      // 记录开始时间
      long long start_time = current_timestamp();

      // 处理请求
      web(socketfd, hit);

      // 记录结束时间
      long long end_time = current_timestamp();

      // 计算消耗时间
      long long elapsed_time = end_time - start_time;
      
      // 打印每个子进程的消耗时间
			printf("Child process %d: %lld microseconds\n", getpid(), elapsed_time);

      // 使用POSIX信号量保护共享内存的写操作
      sem_wait(semaphore);
      *total_time += elapsed_time;
      sem_post(semaphore);

      exit(0); // 子进程处理完请求后退出
    }
    else // 父进程
    {
      close(socketfd); // 父进程不需要已接受的套接字
      waitpid(-1, NULL, WNOHANG); // 非阻塞等待回收僵尸进程
    }
  }

  // ... (现有的代码)
  // 清理POSIX信号量
  sem_close(semaphore);
  sem_unlink("/my_semaphore");

  // 清理共享内存
  munmap(total_time, sizeof(long long));
  shm_unlink("/my_shared_memory");

  return 0;
}

```

![image-20231118215746656](C:/Users/lemon/AppData/Roaming/Typora/typora-user-images/image-20231118215746656.png)

* 问题记录: /usr/bin/ld: webserver.o: in function `main': webserver.c:(.text+0x7b7): undefined reference to `sem_open' /usr/bin/ld: webserver.c:(.text+0x7f6): undefined reference to `shm_open' /usr/bin/ld

* 解决方案：

  ```makefile
  CC = gcc
  CFLAGS = -Wall
  LDFLAGS = -pthread -lrt

* 问题记录： 如何通过Ctrl+C来退出服务，并在退出时输出所有子进程的消耗时间之和。
* 可以使用信号处理机制来捕获Ctrl+C（SIGINT）信号，并在信号处理函数中设置退出标志。然后，可以在主循环中检查该退出标志，以便在Ctrl+C被触发时退出服务并输出所有子进程的消耗时间之和。

```c
#include <signal.h>

// 添加全局变量和信号处理函数
volatile sig_atomic_t stop_server = 0;
pid_t child_processes[64]; // 假设最多64个子进程

void handle_signal(int signo)
{
    if (signo == SIGINT)
    {
        stop_server = 1;

        // 向所有子进程发送SIGTERM信号
        for (int i = 0; i < 64; i++)
        {
            if (child_processes[i] > 0)
            {
                kill(child_processes[i], SIGTERM);
            }
        }
    }
}

// ... (其他代码)

int main(int argc, char **argv)
{
    // ... (其他代码)

    // 设置SIGINT信号处理函数
    signal(SIGINT, handle_signal);

    for (hit = 1; !stop_server; hit++)
    {
        // ... (主循环的代码)

        if ((pid = fork()) < 0)
        {
            logger(ERROR, "system call", "fork", 0);
        }
        else if (pid == 0) // 子进程
        {
            // ... (子进程的代码)

            exit(0); // 子进程处理完请求后退出
        }
        else // 父进程
        {
            child_processes[hit - 1] = pid; // 记录子进程的PID
            close(socketfd);                // 父进程不需要已接受的套接字
            waitpid(-1, NULL, WNOHANG);     // 非阻塞等待回收僵尸进程
        }
    }

    // 输出所有子进程消耗时间之和
    printf("Total processing time for all child processes: %lld microseconds\n", *total_time);

    // 清理POSIX信号量
    sem_close(semaphore);
    sem_unlink("/my_semaphore");

    // 清理共享内存
    munmap(total_time, sizeof(long long));
    shm_unlink("/my_shared_memory");

    return 0;
}

```



######  题目3：使用http_load来测试当前设计的多进程WebServer服务性能

![image-20231118204329169](C:/Users/lemon/AppData/Roaming/Typora/typora-user-images/image-20231118204329169.png)

**其比单进程Web服务性能提高的原因：**

1. **并行处理请求：** 多进程允许同时处理多个请求，而不是一个接一个地处理。这可以显著提高并发性能，尤其是在有大量同时到达的请求时。
2. **系统资源的更好利用：** 多进程可以更好地利用多核系统的资源。每个进程都可以在一个独立的CPU核上执行，从而提高整体性能。
3. **防止阻塞：** 单进程模型可能会因为一个长时间运行的请求而阻塞其他请求的处理。而使用多进程，每个请求都在独立的进程中执行，一个进程的阻塞不会影响其他进程。

**在多进程Web服务器的情况下，潜在的瓶颈和优化机会：**

1. **进程间通信开销：** 在多进程模型中，进程间通信（IPC）可能引入一些开销。这包括通过共享内存传递数据、使用信号量进行同步等。你可以考虑优化进程间通信的方式，或者尽量减少它们的频率。
2. **文件系统访问：** 如果Web服务器在处理请求时频繁地访问文件系统，文件系统的性能可能成为瓶颈。使用缓存机制、减少对磁盘的访问次数，以及使用更高性能的文件系统都是可能的优化点。
3. **共享资源竞争：** 如果多个进程竞争共享资源（如共享内存区域），可能会引起性能问题。使用适当的同步机制，如互斥锁，来避免竞争条件。
