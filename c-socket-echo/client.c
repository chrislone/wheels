#include "common.h"

int main(int argc, char **argv)
{
    struct sockaddr_in serveraddr;

    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == -1)
    {
        printf("socket error\n");
        return 1;
    }

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    // htons 函数为 16 位无符号整数执行主机字节序到网络字节序的转换
    // 每台主机的最大端口数量为 2^16 = 65536，所以 16 位够用
    // htons => host to net short
    serveraddr.sin_port = htons(PORT);
    // htonl 函数为 32 位无符号整数执行主机字节序到网络字节序的转换
    // IPv4 的地址为 32 位无符号数，所以需要使用 htonl 进行转换
    // htonl => host to net long
    // 此处 INADDR_ANY 表示 32 位的 0
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(client_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    int connfd;
    if ((connfd = connect(client_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0)
    {
        printf("connect fail\n");
    }

    while (1)
    {
        char buf[MAX_LEN];
        char buf_from_server[MAX_LEN];
        // fgets 函数把 \n 也拷贝进 buf 中
        fgets(buf, MAX_LEN, stdin);
        // sizeof 函数把字符串结尾的 \0 也计算在内
        write(client_sock, buf, sizeof(buf));
        ssize_t n = read(client_sock, buf_from_server, MAX_LEN);
        if (n > 0)
        {
            printf("echo from server: %s", buf_from_server);
        }
    }

    close(client_sock);
    return 0;
}