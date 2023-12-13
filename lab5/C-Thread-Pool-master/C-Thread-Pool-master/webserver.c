#include "thpool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define WEB_ROOT "./" // Directory where the server's resources are located

// Threadpool definitions
threadpool read_msg_threadpool;
threadpool read_file_threadpool;
threadpool send_msg_threadpool;

typedef struct
{
    int client_fd;
    char request[BUFFER_SIZE];
} client_request;

typedef struct
{
    client_request *request;
    char *filename;
    int file_fd;
    struct stat file_stat;       // This will hold the file size and other attributes
    char *file_content;          // Buffer to hold the file's content
    ssize_t file_content_length; // Length of the file content
} response_data;

// Function prototypes
void read_request(int client_fd);
void read_file_and_respond(client_request *request);
void send_response(response_data *data);

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Initialize threadpools
    read_msg_threadpool = thpool_init(4);
    read_file_threadpool = thpool_init(4);
    send_msg_threadpool = thpool_init(4);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d...\n", PORT);

    while (1)
    {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            continue;
        }
        printf("Connection established with %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        // Dispatch to read_msg_threadpool
        thpool_add_work(read_msg_threadpool, (void (*)(void *))read_request, (void *)(intptr_t)client_fd);
    }

    // Cleanup
    thpool_destroy(read_msg_threadpool);
    thpool_destroy(read_file_threadpool);
    thpool_destroy(send_msg_threadpool);
    close(server_fd);
    return 0;
}
void read_request(int client_fd)
{
    client_request *request = malloc(sizeof(client_request));
    request->client_fd = client_fd;
    int bytes_read = recv(client_fd, request->request, BUFFER_SIZE, 0);
    if (bytes_read <= 0)
    {
        close(client_fd);
        free(request);
        return;
    }
    thpool_add_work(read_file_threadpool, (void (*)(void *))read_file_and_respond, request);
}

// Function to read file and respond
void read_file_and_respond(client_request *request)
{
    // Extract the requested URL from the HTTP request here
    // For simplicity, let's assume the URL is the second word in the request
    char *token = strtok(request->request, " ");
    token = strtok(NULL, " ");
    if (token == NULL)
    {
        close(request->client_fd);
        free(request);
        return;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s%s", WEB_ROOT, token);

    int file_fd = open(filepath, O_RDONLY);
    if (file_fd < 0)
    {
        // File not found, handle error
        close(request->client_fd);
        free(request);
        return;
    }

    // if (fstat(file_fd, &request->file_stat) < 0)
    // {
    //     // Error in getting file stats
    //     close(file_fd);
    //     close(request->client_fd);
    //     free(request);
    //     return;
    // }

    response_data *data = malloc(sizeof(response_data));
    data->request = request;
    data->file_fd = file_fd;
    data->filename = strdup(filepath);

    // Get file size using fstat
    if (fstat(file_fd, &data->file_stat) < 0)
    {
        // Handle error
        close(file_fd);
        close(request->client_fd);
        free(request);
        free(data);
        return;
    }

    // Allocate buffer for the file content
    data->file_content = malloc(data->file_stat.st_size);
    if (data->file_content == NULL)
    {
        // Handle memory allocation error
    }
    data->file_content_length = read(file_fd, data->file_content, data->file_stat.st_size);
    if (data->file_content_length < 0)
    {
        // Handle read error
    }

    thpool_add_work(send_msg_threadpool, (void (*)(void *))send_response, data);
}

// Function to send response
void send_response(response_data *data)
{
    // Send HTTP header before sending the file
    char header[BUFFER_SIZE];
    sprintf(header, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", data->file_content_length);
    send(data->request->client_fd, header, strlen(header), 0);

    // Send the file content
    ssize_t sent_bytes = send(data->request->client_fd, data->file_content, data->file_content_length, 0);
    if (sent_bytes < 0)
    {
        perror("Error sending file content");
    }

    // Clean up
    close(data->file_fd);
    close(data->request->client_fd);
    free(data->filae_content);
    free(data->request);
    free(data->filename);
    free(data);
}
