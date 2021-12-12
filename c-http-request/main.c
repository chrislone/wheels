#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

#define MAXLEN 4096

int main(int argc, char **argv)
{
    struct sockaddr_in server_addr;
    uint16_t port = 80;
    char server_ip[32];

    if (argc < 3)
    {
        puts("missing params, exit");
        exit(0);
    }
    sscanf(argv[1], "%s", server_ip);
    sscanf(argv[2], "%hd", &port);

    int client_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock_fd == -1)
    {
        error("socket error");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    char request_buf[MAXLEN];
    char *request_buf_format = "GET / HTTP/1.0\r\nHost: %s\r\nAccept: */*\r\nConnection: close\r\n\r\n";
    sprintf(request_buf, request_buf_format, server_ip);

    int connfd;
    connfd = connect(client_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (connfd < 0)
    {
        error("connect fail");
    }

    int total = strlen(request_buf);
    int sent = 0;
    size_t left = total;
    while (left > 0)
    {
        // 从第二个参数（指针）的位置开始读第三个参数数量字节的数据到第一个参数（文件描述符）
        // 所表示的文件中。
        // 如果错误返回 -1，成功则返回写入成功的数量
        ssize_t bytes = write(client_sock_fd, request_buf + sent, left);
        if (bytes == 0)
        {
            break;
        }
        else if (bytes < 0)
        {
            error("ERROR writing message to socket");
        }
        left -= bytes;
        sent += bytes;
    }

    char return_buf[MAXLEN];
    memset(return_buf, 0, sizeof(return_buf));
    ssize_t total_to_read = sizeof(return_buf) - 1;
    ssize_t received = 0;
    while (1)
    {
        // 从第一个参数指定的文件中读取第三个参数指定数量的字节到第二个参数
        // 指定的地址中
        // 如果第三个参数为 0 则 read() 返回 0
        // 读取成功则返回读取成功的数量，读取失败则返回 -1
        ssize_t bytes = read(client_sock_fd, return_buf + received, total_to_read - received);
        if (bytes == 0)
        {
            break;
        }
        else if (bytes < 0)
        {
            error("ERROR reading message from socket");
        }
        received += bytes;
    }
    close(client_sock_fd);
    printf("response from server: \n\n%s\n", return_buf);

    return 0;
}