### 开启服务:：后台有进程; 浏览器端一直刷新但是没有数据。杀掉进程：浏览器端断联



debug 

* 修改webserver.c的web()函数为nweb.c的web();  重新make; ---> 浏览器转圈但是没有数据
* 杀掉进程。修改webserver.c的main()为nweb.c的main()  重新make ---> connected 有favicon不转圈，但是网页无法显示
* 修改webserver.c的logger()为nweb.c的logger()  重新make ---> 完全正常加载

~~太奇怪了，最后竟然是logger：D。算了，继续测试~~

* 修改webserver.c的web()函数为webserver.c的web() --> 完全正常加载
* 修改webserver.c的main()函数为webserver.c的main() ---> 浏览器转圈但是没有数据
* 001 ---> connected 有falcon，网页不转圈，但是网页无法显示
* 100 ---> 浏览器转圈但是没有数据



结论：

web()不影响服务端传送数据。与假设正好相反。起初是认为web()的原因导致了无法发送数据。

main()影响favicon

​	    

继续深入函数内部🥰

```diff web()```

```
if (type == ERROR || type == NOTFOUND || type == FORBIDDEN)
    exit(3);
```



```diff main()```

```c


/* Become deamon + unstopable and no zombies children (= no wait()) 
选中的代码是在 Unix-based 系统中创建守护进程的常见模式。守护进程是一个脱离控制终端并在后台运行的进程，通常负责处理周期性的服务请求。

下面是每行代码的解释：

if (fork() != 0) return 0;：这行代码使用 fork() 创建一个子进程。如果当前进程是父进程（fork 调用返回子进程的 pid，不为0），则结束进程，让子进程在后台继续运行。

(void)signal(SIGCLD, SIG_IGN);：这行代码告诉系统忽略 SIGCLD（子状态已更改）信号。这样做是因为进程不关心其子进程是否停止或终止。

(void)signal(SIGHUP, SIG_IGN);：这行代码告诉系统忽略 SIGHUP（控制终端挂起或控制进程死亡）信号。这使得进程即使用户注销也能继续运行。

for (i = 0; i < 32; i++) (void)close(i);：这个循环关闭所有的文件描述符，直到32（一个常见的最大值）。这样做是为了防止守护进程保持任何不必要的文件打开，可能耗尽系统的文件描述符限制。

(void)setpgrp();：这行代码创建一个新的进程组，并使当前进程成为新组的领导者。这样做是为了确保进程不受发送到原始进程组的信号的影响。

logger(LOG, "nweb starting", argv[1], getpid());：这行代码记录了一个消息，表示守护进程已经开始。logger 函数应该在代码的其他地方定义。*/


  if (fork() != 0)
    return 0;                    /* parent returns OK to shell */
  (void)signal(SIGCLD, SIG_IGN); /* ignore child death */
  (void)signal(SIGHUP, SIG_IGN); /* ignore terminal hangups */
  for (i = 0; i < 32; i++)
    (void)close(i); /* close open files */
  (void)setpgrp();  /* break away from process group */
  logger(LOG, "nweb starting", argv[1], getpid());
  
```

```c
/*这段代码是一个无限循环，它监听一个网络端口，等待客户端连接。当有客户端连接时，它会创建一个子进程来处理客户端请求。子进程会调用 web 函数来处理请求，而父进程则会关闭与客户端的连接，继续监听端口等待下一个客户端连接。这段代码是一个无限循环，它监听一个网络端口，等待客户端连接。当有客户端连接时，它会创建一个子进程来处理客户端请求。子进程会调用 web 函数来处理请求，而父进程则会关闭与客户端的连接，继续监听端口等待下一个客户端连接。*/



for (hit = 1;; hit++)
  {
    length = sizeof(cli_addr);
    if ((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
      logger(ERROR, "system call", "accept", 0);
    if ((pid = fork()) < 0)
    {
      logger(ERROR, "system call", "fork", 0);
    }
    else
    {
      if (pid == 0)
      { /* child */
        (void)close(listenfd);
        web(socketfd, hit); /* never returns */
      }
      else
      { /* parent */
        (void)close(socketfd);
      }
    }
  }
```



加了```if (type == ERROR || type == NOTFOUND || type == FORBIDDEN)
    exit(3)```; 但是不同于101，出现了不同的现象  ---> connected 有favicon不转圈，但是网页无法显示 --> 和001一样的了

对于logger()， if(...)从1-->0和从0-->1是完全不同的。😅

**玄学问题，先不去管**





1. I tried to start the sever as below and used a browser to test it. The web page did appeared, but when i reload it, the browser say:Unable to connect



Github copilot：

>The issue you're experiencing might be due to the fact that the server process is not properly handling multiple connections. In the current implementation, the server handles one connection at a time and then exits. This is because the `web()` function, which is responsible for handling the client connection, calls `exit(1)` at the end. This causes the server process to terminate after serving one request.
>
>To fix this, you can fork a new process for each connection, allowing the server to handle multiple connections simultaneously. Here's how you can modify the main loop in your server code:

```c++
for(hit=1; ;hit++) {

  length = sizeof(cli_addr);

  if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)

    logger(ERROR,"system call","accept",0);

  pid = fork();

  if (pid < 0) {

    logger(ERROR, "system call", "fork", 0);

  } else if (pid == 0) {

    close(listenfd);

    web(socketfd,hit); /* never returns */

    exit(0);

  } else {

    close(socketfd);

  }

}

```



2. I know this is a single-connetion server. But on the single connection. why the image can't be loaded

GitHub Copilot:

> There could be several reasons why the image is not loading on the single connection server. One possible reason is that the server is not sending the correct HTTP response headers for the image file. Another possible reason is that the image file is not located in the correct directory or the file name is incorrect. It's also possible that there is an error in the client-side code that is preventing the image from being displayed. Without more information, it's difficult to determine the exact cause of the issue.





