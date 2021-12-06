#include "common.h"

int main(int argc, char **argv)
{
    struct sockaddr_in name;

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
    {
        printf("socket error\n");
        return 1;
    }

    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    // htons 函数为 16 位无符号整数执行主机字节序到网络字节序的转换
    // 每台主机的最大端口数量为 2^16 = 65536，所以 16 位够用
    // htons => host to net short
    name.sin_port = htons(PORT);
    // htonl 函数为 32 位无符号整数执行主机字节序到网络字节序的转换
    // IPv4 的地址为 32 位无符号数，所以需要使用 htonl 进行转换
    // htonl => host to net long
    // 此处 INADDR_ANY 表示 32 位的 0
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(server_sock, (struct sockaddr *)&name, sizeof(name));

    // 第二个参数为 backlog，表示可接受的最大连接数
    listen(server_sock, 5);
    printf("listening port: %d\n", PORT);

    struct sockaddr_in client_name;
    socklen_t client_name_len = sizeof(client_name);

    // 迭代 accept
    while (1)
    {
        int connfd = accept(server_sock, (struct sockaddr *)&client_name, &client_name_len);
        printf("connect from client\n");

        while (1)
        {
            char buf[MAX_LEN];
            // read 一个 socket 文件描述符会阻塞
            ssize_t read_len = read(connfd, buf, MAX_LEN);
            if (read_len < 0 || read_len == 0)
            {
                break;
            }
            else if (read_len > 0)
            {
                write(connfd, buf, strlen(buf) + 1);
            }
        }

        close(connfd);
    }
    return 0;
}
