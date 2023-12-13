#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
long long current_timestamp()
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
    {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    return ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

// ... (rest of your code)

volatile sig_atomic_t stop_server = 0;

void handle_signal(int signo)
{
    if (signo == SIGINT)
    {
        stop_server = 1;
    }
}

sem_t *semaphore;
long long *total_time;

#define VERSION 23
#define BUFSIZE 8096
#define ERROR 42
#define LOG 44
#define FORBIDDEN 403
#define NOTFOUND 404

struct
{
    char *ext;
    char *filetype;
} extensions[] = {
    {"gif", "image/gif"},
    {"jpg", "image/jpg"},
    {"jpeg", "image/jpeg"},
    {"png", "image/png"},
    {"ico", "image/ico"},
    {"zip", "image/zip"},
    {"gz", "image/gz"},
    {"tar", "image/tar"},
    {"htm", "text/html"},
    {"html", "text/html"},
    {0, 0}};

void logger(int type, char *s1, char *s2, int socket_fd)
{
    int fd;
    char logbuffer[BUFSIZE * 2];

    switch (type)
    {
    case ERROR:
        (void)sprintf(logbuffer, "ERROR: %s:%s Errno=%d exiting pid=%d", s1, s2, errno, getpid());
        break;
    case FORBIDDEN:
        (void)write(socket_fd, "HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type, or operation is not allowed on this simple static file webserver.\n</body></html>\n", 271);
        (void)sprintf(logbuffer, "FORBIDDEN: %s:%s", s1, s2);
        break;
    case NOTFOUND:
        (void)write(socket_fd, "HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n", 224);
        (void)sprintf(logbuffer, "NOT FOUND: %s:%s", s1, s2);
        break;
    case LOG:
        (void)sprintf(logbuffer, " INFO: %s:%s:%d", s1, s2, socket_fd);
        break;
    }

    if ((fd = open("webserver.log", O_CREAT | O_WRONLY | O_APPEND, 0644)) >= 0)
    {
        (void)write(fd, logbuffer, strlen(logbuffer));
        (void)write(fd, "\n", 1);
        (void)close(fd);
    }
}

void web(int fd, int hit)
{
    int j, file_fd, buflen;
    long i, ret, len;
    char *fstr;
    static char buffer[BUFSIZE + 1];

    ret = read(fd, buffer, BUFSIZE);
    if (ret == 0 || ret == -1)
    {
        logger(FORBIDDEN, "failed to read browser request", "", fd);
    }
    if (ret > 0 && ret < BUFSIZE)
        buffer[ret] = 0;
    else
        buffer[0] = 0;
    for (i = 0; i < ret; i++)
        if (buffer[i] == '\r' || buffer[i] == '\n')
            buffer[i] = '*';
    logger(LOG, "request", buffer, hit);
    if (strncmp(buffer, "GET ", 4) && strncmp(buffer, "get ", 4))
    {
        logger(FORBIDDEN, "Only simple GET operation supported", buffer, fd);
    }
    for (i = 4; i < BUFSIZE; i++)
    {
        if (buffer[i] == ' ')
        {
            buffer[i] = 0;
            break;
        }
    }
    for (j = 0; j < i - 1; j++)
        if (buffer[j] == '.' && buffer[j + 1] == '.')
        {
            logger(FORBIDDEN, "Parent directory (..) path names not supported", buffer, fd);
        }
    if (!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6))
        (void)strcpy(buffer, "GET /index.html");

    buflen = strlen(buffer);
    fstr = (char *)0;
    for (i = 0; extensions[i].ext != 0; i++)
    {
        len = strlen(extensions[i].ext);
        if (!strncmp(&buffer[buflen - len], extensions[i].ext, len))
        {
            fstr = extensions[i].filetype;
            break;
        }
    }
    if (fstr == 0)
        logger(FORBIDDEN, "file extension type not supported", buffer, fd);

    if ((file_fd = open(&buffer[5], O_RDONLY)) == -1)
    {
        logger(NOTFOUND, "failed to open file", &buffer[5], fd);
    }
    logger(LOG, "SEND", &buffer[5], hit);
    len = (long)lseek(file_fd, (off_t)0, SEEK_END);
    (void)lseek(file_fd, (off_t)0, SEEK_SET);
    (void)sprintf(buffer, "HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n\n", VERSION, len, fstr);
    logger(LOG, "Header", buffer, hit);
    (void)write(fd, buffer, strlen(buffer));

    while ((ret = read(file_fd, buffer, BUFSIZE)) > 0)
    {
        (void)write(fd, buffer, ret);
    }
    sleep(1);
    close(fd);
}

int main(int argc, char **argv)
{
    signal(SIGINT, handle_signal);
    sem_unlink("/my_semaphore");
    semaphore = sem_open("/my_semaphore", O_CREAT | O_EXCL, 0644, 1);
    if (semaphore == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

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

    int i, port, pid, listenfd, socketfd, hit;
    socklen_t length;
    static struct sockaddr_in cli_addr;
    static struct sockaddr_in serv_addr;

    if (argc < 3 || argc > 3 || !strcmp(argv[1], "-?"))
    {
        (void)printf("hint: nweb Port-Number Top-Directory\t\tversion %d\n\n"
                     "\tnweb is a small and very safe mini web server\n"
                     "\tnweb only servers out file/web pages with extensions named below\n"
                     "\t and only from the named directory or its sub-directories.\n"
                     "\tThere is no fancy features = safe and secure.\n\n"
                     "\tExample: nweb 8181 /home/nwebdir &\n\n"
                     "\tOnly Supports:",
                     VERSION);
        for (i = 0; extensions[i].ext != 0; i++)
            (void)printf(" %s", extensions[i].ext);

        (void)printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n"
                     "\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n"
                     "\tNo warranty given or implied\n\tNigel Griffiths nag@uk.ibm.com\n");
        exit(0);
    }
    if (!strncmp(argv[2], "/", 2) || !strncmp(argv[2], "/etc", 5) ||
        !strncmp(argv[2], "/bin", 5) || !strncmp(argv[2], "/lib", 5) ||
        !strncmp(argv[2], "/tmp", 5) || !strncmp(argv[2], "/usr", 5) ||
        !strncmp(argv[2], "/dev", 5) || !strncmp(argv[2], "/sbin", 6))
    {
        (void)printf("ERROR: Bad top directory %s, see nweb -?\n", argv[2]);
        exit(3);
    }
    if (chdir(argv[2]) == -1)
    {
        (void)printf("ERROR: Can't Change to directory %s\n", argv[2]);
        exit(4);
    }

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        logger(ERROR, "system call", "socket", 0);
    port = atoi(argv[1]);
    if (port < 0 || port > 60000)
        logger(ERROR, "Invalid port number (try 1->60000)", argv[1], 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        logger(ERROR, "system call", "bind", 0);
    if (listen(listenfd, 64) < 0)
        logger(ERROR, "system call", "listen", 0);
    for (hit = 1; !stop_server; hit++)
    {
        length = sizeof(cli_addr);
        if ((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
            logger(ERROR, "system call", "accept", 0);

        if ((pid = fork()) < 0)
        {
            logger(ERROR, "system call", "fork", 0);
        }
        else if (pid == 0)
        {
            close(listenfd);

            int ret;
            char buffer[BUFSIZE];

            long long start_time_socket_read = current_timestamp();
            ret = read(socketfd, buffer, BUFSIZE);
            long long end_time_socket_read = current_timestamp();
            long long elapsed_time_socket_read = end_time_socket_read - start_time_socket_read;
            printf("Child process %d - Socket Read: %lld microseconds\n", getpid(), elapsed_time_socket_read);

            long long start_time_socket_send = current_timestamp();
            ret = write(socketfd, buffer, ret);
            long long end_time_socket_send = current_timestamp();
            long long elapsed_time_socket_send = end_time_socket_send - start_time_socket_send;
            printf("Child process %d - Socket Send: %lld microseconds\n", getpid(), elapsed_time_socket_send);

            int file_fd; // Declare file descriptor
            if ((file_fd = open(&buffer[5], O_RDONLY)) == -1)
            { /* open the file for reading */
                logger(NOTFOUND, "failed to open file", &buffer[5], socketfd);
                exit(1);
            }
            long long start_time_file_read = current_timestamp();
            while ((ret = read(file_fd, buffer, BUFSIZE)) > 0)
            {
                // Process file data
            }
            long long end_time_file_read = current_timestamp();
            long long elapsed_time_file_read = end_time_file_read - start_time_file_read;
            printf("Child process %d - File Read: %lld microseconds\n", getpid(), elapsed_time_file_read);

            long long start_time_log_write = current_timestamp();
            logger(LOG, "SEND", &buffer[5], hit);
            long long end_time_log_write = current_timestamp();
            long long elapsed_time_log_write = end_time_log_write - start_time_log_write;
            printf("Child process %d - Log Write: %lld microseconds\n", getpid(), elapsed_time_log_write);

            sem_wait(semaphore);
            *total_time += (elapsed_time_socket_read + elapsed_time_socket_send + elapsed_time_file_read + elapsed_time_log_write);
            sem_post(semaphore);

            exit(0);
        }
        else
        {
            close(socketfd);
            int status;
            waitpid(pid, &status, 0); // Wait for the child process to finish
        }
    }

    printf("Total processing time for all child processes: %lld microseconds\n", *total_time);

    sem_close(semaphore);
    sem_unlink("/my_semaphore");

    munmap(total_time, sizeof(long long));
    shm_unlink("/my_shared_memory");

    return 0;
}
