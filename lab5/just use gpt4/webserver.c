#include "threadpool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_BUFFER 1024
#define QUEUE_SIZE 10
#define THREAD_COUNT 5

typedef struct filename_node
{
    char *filename;
    int socket;
    struct filename_node *next;
} filename_node_t;

typedef struct
{
    filename_node_t *front;
    filename_node_t *rear;
    pthread_mutex_t lock;
} filename_queue_t;

typedef struct msg_node
{
    char *file_content;
    int socket;
    struct msg_node *next;
} msg_node_t;

typedef struct
{
    msg_node_t *front;
    msg_node_t *rear;
    pthread_mutex_t lock;
} msg_queue_t;

// Thread pool task function prototypes
void read_msg_task(void *arg);
void read_file_task(void *arg);
void send_msg_task(void *arg);

// Initialize queues and thread pools
filename_queue_t filename_queue;
msg_queue_t msg_queue;
threadpool_t *read_msg_threadpool;
threadpool_t *read_file_threadpool;
threadpool_t *send_msg_threadpool;

// Define functions for each thread pool task
void read_msg_task(void *arg);
void read_file_task(void *arg);
void send_msg_task(void *arg);
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s [port_number] [directory]\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    char *directory = argv[2];
    chdir(directory); // Change to the specified directory

    // Initialize server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Socket bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, QUEUE_SIZE) < 0)
    {
        perror("Socket listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Create thread pools
    threadpool_t *read_msg_threadpool = threadpool_create(THREAD_COUNT);
    threadpool_t *read_file_threadpool = threadpool_create(THREAD_COUNT);
    threadpool_t *send_msg_threadpool = threadpool_create(THREAD_COUNT);

    printf("Server is running on port %d\n", port);

    while (1)
    {
        int client_socket = accept(server_fd, NULL, NULL);
        if (client_socket < 0)
        {
            perror("Socket accept failed");
            continue;
        }

        // Add a task to read_msg_threadpool
        threadpool_add_task(read_msg_threadpool, read_msg_task, (void *)(intptr_t)client_socket);
    }

    // Cleanup (this part will be reached only when the server is shutting down)
    threadpool_destroy(read_msg_threadpool);
    threadpool_destroy(read_file_threadpool);
    threadpool_destroy(send_msg_threadpool);
    close(server_fd);

    return 0;
}
void read_msg_task(void *arg)
{
    int client_socket = (int)(intptr_t)arg;
    char buffer[MAX_BUFFER];

    // Read message from client
    int read_bytes = read(client_socket, buffer, MAX_BUFFER - 1);
    if (read_bytes <= 0)
    {
        close(client_socket);
        return;
    }

    buffer[read_bytes] = '\0';

    // Enqueue the filename and socket to filename_queue
    enqueue_filename(&filename_queue, buffer, client_socket);
}
void read_file_task(void *arg)
{
    filename_node_t *request = dequeue_filename(&filename_queue);
    if (request == NULL)
    {
        return;
    }

    // Read file content based on request->filename
    char *file_content = ...; // Implement file reading logic

    // Enqueue file content to msg_queue
    enqueue_msg(&msg_queue, file_content, request->socket);

    // Free the request node
    free(request->filename);
    free(request);
}
void read_file_task(void *arg)
{
    filename_node_t *request = dequeue_filename(&filename_queue);
    if (request == NULL)
    {
        return;
    }

    // Read file content based on request->filename
    char *file_content = ...; // Implement file reading logic

    // Enqueue file content to msg_queue
    enqueue_msg(&msg_queue, file_content, request->socket);

    // Free the request node
    free(request->filename);
    free(request);
}
void send_msg_task(void *arg)
{
    msg_node_t *msg = dequeue_msg(&msg_queue);
    if (msg == NULL)
    {
        return;
    }

    // Send the file content to client
    write(msg->socket, msg->file_content, strlen(msg->file_content));

    // Close the socket after sending the message
    close(msg->socket);

    // Free the message node
    free(msg->file_content);
    free(msg);
}
