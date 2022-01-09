#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

void error(const char *msg) {
    perror(msg);
    exit(0);
}

#define MAXLEN 8192

void construct_response(char *res, char *method);

void read_line(void *dest, void *from);

void get_http_method(int fd, char *method_pointer);

int main(int argc, char **argv) {
    struct sockaddr_in server_addr;
    // 16 位无符号整型
    uint16_t PORT;

    if (argc < 2) {
        puts("missing params, exit.(./main <port>)");
        exit(0);
    }
    sscanf(argv[1], "%hu", &PORT);

    int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock_fd == -1) {
        error("socket error");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(server_sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    listen(server_sock_fd, 5);
    printf("listening port: %d\n", PORT);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (1) {
        char response[MAXLEN];
        int connfd = accept(server_sock_fd, (struct sockaddr *) &client_addr, &client_addr_len);
        printf("connect from client\n\n");
        char method[10];
        get_http_method(connfd, method);
        construct_response(response, method);
        puts(response);
        write(connfd, response, strlen(response));
        close(connfd);
    }

    return 0;
}

void construct_response(char *res, char *method) {
    char body_format[] = "<!DOCTYPE html>\r\n<html><body><div>You are using %s method.</div></body></html>\r\n";
    char body[200];
    char content_length_str[MAXLEN];

    //  发送格式化输出到第一个参数所指向的字符串
    sprintf(body, body_format, method);
    sprintf(content_length_str, "%zu", strlen(body));
    memset(res, 0, sizeof(*res));

    strcat(res, "HTTP/1.1 200 OK\r\n");
    strcat(res, "Server: main/0.0.1\r\n");
    strcat(res, "Content-Type: text/html\r\n");
    strcat(res, "Content-Length: ");
    strcat(res, content_length_str);
    strcat(res, "\r\n");
    strcat(res, "Connection: close\r\n");
    strcat(res, "Accept-Ranges: bytes\r\n");
    strcat(res, "\r\n");
    strcat(res, body);
}

// 从 from 中读取第一行到 dest
void read_line(void *dest, void *from) {
    char *buf = from;
    char *dest_buf = dest;
    while (*buf != '\0') {
        if (*buf == '\n') {
            break;
        }
        *dest_buf++ = *buf;
        buf++;
    }
    *dest_buf = 0;
}

void get_http_method(int fd, char *method_pointer) {
    char buf[MAXLEN];
    char line_buf[MAXLEN];
    ssize_t bytes = read(fd, buf, MAXLEN);
    if (bytes > 0) {
        read_line(line_buf, buf);
        sscanf(line_buf, "%s", method_pointer);
    }
}