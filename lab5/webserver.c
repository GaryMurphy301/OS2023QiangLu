#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "threadpool.h"
#include <stdbool.h>
#define BUFFER_SIZE 1024

void send_msg_callback(void *arg);
void read_file_callback(void *arg);
struct threadpool *read_msg_threadpool;
struct threadpool *read_file_threadpool;
struct threadpool *send_msg_threadpool;

// 结构体1：用于在 read_msg_callback 和 read_file_callback 之间传递参数
struct ReadFileCallbackArg
{
    int client_socket;
    char file_path[100];
};

// 结构体2：用于在 read_file_callback 和 send_msg_callback 之间传递参数
struct SendMsgCallbackArg
{
    int client_socket;
    char *file_content;
};

void read_msg_callback(void *arg)
{
    int client_socket = *((int *)arg);
    free(arg); // 释放传递给线程的参数内存

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    // 接收客户端的消息
    bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received < 0)
    {
        perror("Error receiving data from client");
        close(client_socket);
        return;
    }

    // 解析请求消息（假设消息格式为 "GET /<filename>"）
    char file_path[100];
    if (sscanf(buffer, "GET /%s", file_path) != 1)
    {
        // 解析失败，关闭客户端连接
        close(client_socket);
        return;
    }

    // 将文件路径和客户端套接字作为参数提交给下一个线程池
    struct ReadFileCallbackArg *callback_arg = (struct ReadFileCallbackArg *)malloc(sizeof(struct ReadFileCallbackArg));
    callback_arg->client_socket = client_socket;
    strcpy(callback_arg->file_path, file_path);

    threadpool_add_job(read_file_threadpool, read_file_callback, (void *)callback_arg);
}

void read_file_callback(void *arg)
{
    struct ReadFileCallbackArg *callback_arg = (struct ReadFileCallbackArg *)arg;
    int client_socket = callback_arg->client_socket;
    const char *file_path = callback_arg->file_path;
    free(callback_arg); // 释放传递给线程的参数内存

    // 打开文件
    FILE *file = fopen(file_path, "rb");
    if (file == NULL)
    {
        // 如果文件打开失败，发送 404 Not Found 响应给客户端
        const char *not_found_response = "HTTP/1.1 404 Not Found\r\n\r\n";
        send(client_socket, not_found_response, strlen(not_found_response), 0);
        close(client_socket);
        return;
    }

    // 读取文件内容
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    size_t total_bytes_read = 0;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        total_bytes_read += bytes_read;
    }

    fclose(file);

    // 将文件内容和客户端套接字作为参数提交给下一个线程池
    struct SendMsgCallbackArg *send_msg_callback_arg = (struct SendMsgCallbackArg *)malloc(sizeof(struct SendMsgCallbackArg));
    send_msg_callback_arg->client_socket = client_socket;
    send_msg_callback_arg->file_content = (char *)malloc(total_bytes_read);

    // 将文件内容读入内存
    memcpy(send_msg_callback_arg->file_content, buffer, total_bytes_read);

    threadpool_add_job(send_msg_threadpool, send_msg_callback, (void *)send_msg_callback_arg);
}

void send_msg_callback(void *arg)
{
    struct SendMsgCallbackArg *callback_arg = (struct SendMsgCallbackArg *)arg;
    int client_socket = callback_arg->client_socket;
    char *file_content = callback_arg->file_content;

    // 发送 HTTP 响应头
    const char *ok_response = "HTTP/1.1 200 OK\r\n\r\n";
    send(client_socket, ok_response, strlen(ok_response), 0);

    // 发送文件内容
    send(client_socket, file_content, strlen(file_content), 0);

    // 关闭客户端套接字和释放内存
    close(client_socket);
    free(file_content);
    free(callback_arg);
}

struct WebServerConfig
{
    int port;
    const char *file_directory;
    int thread_num; // Added: Thread pool size
};

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s [port] [file_directory] [thread_num]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Extract command line arguments
    struct WebServerConfig config;
    config.port = atoi(argv[1]);
    config.file_directory = argv[2];
    config.thread_num = atoi(argv[3]);

    // Create the thread pools
    struct threadpool *read_msg_threadpool = threadpool_init(config.thread_num, 10);
    struct threadpool *read_file_threadpool = threadpool_init(config.thread_num, 10);
    struct threadpool *send_msg_threadpool = threadpool_init(config.thread_num, 10);

    // Create a socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config.port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 10) < 0)
    {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    printf("Web server listening on port %d...\n", config.port);

    while (1)
    {
        // Accept a connection
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0)
        {
            perror("Error accepting connection");
            continue;
        }

        // 将任务提交到read_msg_threadpool
        int *client_socket_ptr = (int *)malloc(sizeof(int));
        *client_socket_ptr = client_socket;
        threadpool_add_job(read_msg_threadpool, read_msg_callback, (void *)client_socket_ptr);
    }

    // Close the server socket and destroy the thread pool
    // 关闭服务器套接字和销毁线程池
    close(server_socket);
    threadpool_destroy(read_msg_threadpool);
    threadpool_destroy(read_file_threadpool);
    threadpool_destroy(send_msg_threadpool);

    return 0;
}
