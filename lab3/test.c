// Multithreaded web server
// Compile with: gcc -std=gnu9 -g -o multithread_webserver multithread_webserver.c -lpthread

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
#include <pthread.h>
#include <sys/stat.h>

#define VERSION "23"
#define BUFSIZE 8096
#define ERROR 1
#define FORBIDDEN 403
#define NOTFOUND 404
#define LOG 0

struct
{
    char *ext;
    char *filetype;
} extensions[] = {
    {"gif", "image/gif"},
    {"jpg", "image/jpg"},
    {"jpeg", "image/jpeg"},
    {"png", "image/png"},
    {"ico", "image/x-icon"},
    {"zip", "image/zip"},
    {"gz", "image/gz"},
    {"tar", "image/tar"},
    {0, 0}};

struct webparam
{
    int fd;
    int hit;
};

unsigned long get_file_size(const char *path)
{
    unsigned long filesize = -1;
    struct stat statbuff;

    if (stat(path, &statbuff) >= 0)
    {
        filesize = statbuff.st_size;
    }

    return filesize;
}

void logger(int type, char *s1, char *s2, int socket_fd)
{
    int fd;
    char logbuffer[BUFSIZE * 2];

    switch (type)
    {
    case ERROR:
        sprintf(logbuffer, "ERROR: %s: %s Errno=%d exiting pid=%d", s1, s2, errno, getpid());
        break;
    case FORBIDDEN:
        write(socket_fd, "HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URI, file type, or operation is not allowed on this simple static file web server.\n</body></html>\n", 271);
        sprintf(logbuffer, "FORBIDDEN: %s: %s", s1, s2);
        break;
    case NOTFOUND:
        write(socket_fd, "HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URI was not found on this server.\n</body></html>\n", 224);
        sprintf(logbuffer, "NOT FOUND: %s: %s", s1, s2);
        break;
    case LOG:
        sprintf(logbuffer, "INFO: %s: %s:%d", s1, s2, socket_fd);
        break;
    }

    // No checks here, nothing can be done with a failure anyway
    if ((fd = open("nweb.log", O_CREAT | O_WRONLY | O_APPEND, 0644)) >= 0)
    {
        write(fd, logbuffer, strlen(logbuffer));
        write(fd, "\n", 1);
        close(fd);
    }
}

void *web(void *data)
{
    int fd, hit, j, file_fd, buflen;
    long i, ret, len;
    char *fstr;
    char buffer[BUFSIZE + 1];
    struct webparam *param = (struct webparam *)data;

    fd = param->fd;
    hit = param->hit;

    ret = read(fd, buffer, BUFSIZE); /* Read web request content from the socket */
    if (ret <= 0)
    {
        logger(FORBIDDEN, "Failed to read browser request", "", fd);
    }
    else
    {
        if (ret > 0 && ret < BUFSIZE)
            buffer[ret] = 0;
        else
            buffer[0] = 0;

        for (i = 0; i < ret; i++)
        {
            if (buffer[i] == '\r' || buffer[i] == '\n')
                buffer[i] = '*';
        }

        logger(LOG, "Request", buffer, hit);

        if (strncmp(buffer, "GET", 3) && strncmp(buffer, "get", 3))
        {
            logger(FORBIDDEN, "Only simple GET operation supported", buffer, fd);
        }
        else
        {
            for (i = 4; i < BUFSIZE; i++)
            {
                if (buffer[i] == ' ')
                {
                    buffer[i] = 0;
                    break;
                }
            }

            for (j = 0; j < i - 1; j++)
            {
                if (buffer[j] == '.' && buffer[j + 1] == '.')
                {
                    logger(FORBIDDEN, "Parent directory (..) path names not supported", "", fd);
                    break;
                }
            }

            if (!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6))
            {
                strcpy(buffer, "GET /index.html");
            }

            buflen = strlen(buffer);
            fstr = NULL;

            for (i = 0; extensions[i].ext != 0; i++)
            {
                len = strlen(extensions[i].ext);
                if (!strncmp(&buffer[buflen - len], extensions[i].ext, len))
                {
                    fstr = extensions[i].filetype;
                    break;
                }
            }

            if (fstr == NULL)
            {
                logger(FORBIDDEN, "File extension type not supported", buffer, fd);
            }
            else
            {
                if ((file_fd = open(&buffer[5], O_RDONLY)) == -1)
                {
                    logger(NOTFOUND, "Failed to open file", &buffer[5], fd);
                }
                else
                {
                    logger(LOG, "Send", &buffer[5], hit);
                    len = (long)lseek(file_fd, (off_t)0, SEEK_END);
                    lseek(file_fd, (off_t)0, SEEK_SET);
                    sprintf(buffer, "HTTP/1.1 200 OK\nServer: nweb/%s\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n\n", VERSION, len, fstr);
                    logger(LOG, "Header", buffer, hit);
                    write(fd, buffer, strlen(buffer));

                    while ((ret = read(file_fd, buffer, BUFSIZE)) > 0)
                    {
                        write(fd, buffer, ret);
                        usleep(10000); // Allow time to send a segment of information before closing the socket channel
                    }

                    close(file_fd);
                }
            }
        }
    }

    close(fd); // Release memory
    free(param);

    return NULL;
}

int main(int argc, char **argv)
{
    int i, port, pid, listenfd, socketfd, hit;
    socklen_t length;
    static struct sockaddr in cli addr;  /* static = initialised to zeros */
    static struct sockaddr in serv addr; /* static = initialised to zeros */
if( argc < 3 ll argc > 3 1l !strcmp(argv[1]，“-?") ) ((void)printf("hint:; nweb Port-Number Top-Directory\t\tversion d\n\n"
\tnweb is a small and very safe mini web server\nwwitnweb only servers out file/web pages with extensiong
belowin”
w\t and only from the named directory or its
directories.\n"
"\tThere is no fancy features = safe and secure.\n\nw
"\tExample:nweb 8181 /home/nwebdir &\n\n"n\tonly Supports:"，VERSION);
for(i=0;extensions[i].ext !=0;i++)
(void)printf(”s",extensions[i].ext);
(void)printf("\nltNot Supported: uRis including \"..\", Java, Javaseript
CGI\n"
"\tNot Supported: directories /etc /bin /ib /tmp /usr /de
/sbin \n"
)tNo warranty given or implied\n\tnigel Griffiths nageuk
ibm.com\n”);
exit(0);
if( lstrncmp(argv[2],"/n,2 ) l1 !strncmp(argv[2],"/etc"，5)Istrncmp(argv[2],"/bin",5 ) 1 strncmp(argv(2],"/lib"，5 ) 11Istrncmp(argv[2],"/tmp",5)1 !strncmp(argv[2],"/usr"，5 )1!strncmp(argv[2],"/dev",5 ) l !strncmp(argv[2],"/sbin",6) )[(void)printf("ERROR: Bad top directory s， see nweb -?\n",argv[2]);exit(3);
if(chdir(argv[2])== -1){
        (void)printf("ERROR: Can't Change to directory s\n", argv[2]);
        exit(4);
        if (fork() != 0)
            return 0;
        (void)signal(SIGCLD, .SIG IGN);
        (void)signal(SIGHUP，SIG IGN);
        for (i - 0; i < 32; i++)
            (void)close(i);
        (void)setpgrp();
        logger(LOG, "nweb starting", argv[1], getpid());
        if ((listenfd = socket(AF_INET，SOCK STREAM, 0)) < 0)
            logger(ERROR， "system call", "socket", 0);
        port = atoi(argv[1]);
        if (port < 01l port > 60000)
            logger(ERROR, "Invalid port number (try 1->60000)" argv[1], 0);
        // 初始化线程属性为分离状态
        pthreadattr t attr;
        pthread attr init(&attr);
        pthread attr_setdetachstate(&attr, PTHREAD CREATE DETACHED) : pthread tpth;
        serv addr.sin family = AF INET;
        serv addr.sin_addr.s_addr = htonl(INADDR ANY) : serv addr.sin port = htons(port);
        if (bind(listenfd, (struct sockaddr *)&serv addr; sizeof(serv addr)) < 0)
            logger(ERROR, "system call", "bind", 0);
        if (listen(listenfd, 64) < 0)logger(ERROR,"system call","listen",0):for(hit=l;;hit++)(length= sizeof(cli addr);if((socketfd = accept(listenfd,(struct sockaddr *)&cli addr,&length)) <0)logger(ERROR,"system call","accept",0);webparam *param-malloc(sizeof(webparam));param->hit=hit;
param->fd-socketfd;if(pthread create(spth, &attr，&web,(void*)param)<0)[logger(ERROR,"system calln,"pthread create",0);