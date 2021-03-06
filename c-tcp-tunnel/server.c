#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/select.h>
#include "http.h"

#define BUFFER_SIZE 4096

// 服务器的 socket fd;
unsigned int server_sock_fd;
// 客户端 socket fd;
unsigned int client_sock_fd;
// 远程服务器的 socket fd
unsigned int remote_socket_fd;
// 远程服务器域名，从 http 请求的 host 头中获取
char remote_server_name[100];

uint16_t server_port;
uint16_t remote_server_port = 80;

// extern 变量
method_t method_struct;

int init_server_socket();

int listen_server_socket();

int build_client_sock_fd(void);

int connect_remote_server(void);

int send_header(unsigned int fd, char *header, size_t count);

int use_tunnel(void);

int fd(void);

int main(int argc, char **argv) {
    if (argc < 2) {
        puts("missing params, exit.(./main <port>)");
        exit(0);
    }
    sscanf(argv[1], "%hu", &server_port);

    init_server_socket();
    listen_server_socket();
    if (build_client_sock_fd() != 0) {
        printf("build_client_sock_fd error\n");
        exit(1);
    }

    if (connect_remote_server() != 0) {
        printf("connect_remote_server error\n");
        exit(1);
    }

    use_tunnel();

    close(server_sock_fd);
    return 0;
}

int init_server_socket() {
    server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定 socket
    int bind_return = bind(server_sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (bind_return < 0) {
        perror("build_client_sock_fd () bind error:");
        return 1;
    }

    return 0;
}

int listen_server_socket() {
    // 监听
    int listen_return = listen(server_sock_fd, 5);
    if (listen_return < 0) {
        perror("listen error");
    }
    printf("listening on port: %hu\n", server_port);
    return 0;
}

int build_client_sock_fd(void) {
    int addrLen = sizeof(struct sockaddr);
    char addr[addrLen];
    memset(addr, 0, addrLen);
    socklen_t len = (socklen_t) addrLen;
    // accept 第二个参数可以设置为足够长度的缓冲区来存放客户端的地址信息
    client_sock_fd = accept(server_sock_fd, (struct sockaddr *) addr, &len);
    if (client_sock_fd < 0) {
        perror("build_client_sock_fd () accept error:");
        return 1;
    }
    return 0;
}

int connect_remote_server(void) {
    struct hostent *host_ent;
    char buf[MAX_LEN];
    http_get_header(client_sock_fd, buf);
    memset(remote_server_name, 0, sizeof(remote_server_name));
    char *name = http_get_remote_host(http_header);
    memmove(remote_server_name, name, strlen(name ) + 1);
    host_ent = gethostbyname(remote_server_name);
    if (host_ent == NULL) {
        perror("connect_remote_server() gethostbyname error ");
        return 1;
    }
    struct sockaddr_in remote_server_addr;
    memset(&remote_server_addr, 0, sizeof(remote_server_addr));
    remote_server_addr.sin_family = AF_INET;
    remote_server_addr.sin_port = htons(remote_server_port);
    memcpy(&remote_server_addr.sin_addr.s_addr, host_ent->h_addr_list[0], host_ent->h_length);

    remote_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("connecting...\n");
    int connfd = connect(remote_socket_fd, (struct sockaddr *) &remote_server_addr, sizeof(remote_server_addr));
    if (connfd < 0) {
        perror("connect_remote_server() connect error ");
        return 1;
    } else {
        printf("connect_remote_server() connect success\n");
        char complete[100];
        strcat(complete, method_struct.method);
        strcat(complete, " ");
        strcat(complete, method_struct.url);
        strcat(complete, " ");
        strcat(complete, method_struct.version);
        strcat(complete, "\r\n");
        strcat(complete, http_header);
        send_header(remote_socket_fd, complete, strlen(complete) + 1);
    }
    return 0;
}

int send_header(unsigned int fd, char *header, size_t count) {
    int c = send(fd, header, count, 0);
    return c;
}

int use_tunnel(void) {
    fd_set io;
    char buffer[BUFFER_SIZE];

    for (;;) {
        FD_ZERO(&io);
        FD_SET(client_sock_fd, &io);
        FD_SET(remote_socket_fd, &io);

        if (select(fd(), &io, NULL, NULL, NULL) < 0) {
            perror("use_tunnel: select()");
            break;
        }

        if (FD_ISSET(client_sock_fd, &io)) {
            int count = recv(client_sock_fd, buffer, sizeof(buffer), 0);
            printf("recv client count %d\n", count);
            if (count < 0) {
                perror("use_tunnel: recv(client_sock_fd)");
                close(client_sock_fd);
                close(remote_socket_fd);
                return 1;
            }

            if (count == 0) {
                close(client_sock_fd);
                close(remote_socket_fd);
                return 0;
            }

            send(remote_socket_fd, buffer, count, 0);
        }

        if (FD_ISSET(remote_socket_fd, &io)) {
            int count = recv(remote_socket_fd, buffer, sizeof(buffer), 0);
            printf("recv remote count %d\n", count);
            if (count < 0) {
                perror("use_tunnel: recv(remote_socket_fd)");
                close(client_sock_fd);
                close(remote_socket_fd);
                return 1;
            }

            if (count == 0) {
                close(client_sock_fd);
                close(remote_socket_fd);
                return 0;
            }

            send(client_sock_fd, buffer, count, 0);
        }
    }

    return 0;
}

int fd(void) {
    unsigned int fd = client_sock_fd;
    if (fd < remote_socket_fd) {
        fd = remote_socket_fd;
    }
    return fd + 1;
}