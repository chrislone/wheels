#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <ctype.h>
#include <unistd.h>
#include "http.h"

extern method_t method_struct;

void string_to_loader(char *str);

// 解析 http 头信息，并保存成链表
int http_get_header(unsigned int fd, char buf[]) {
    unsigned int line_size;
    char *ptr = buf;
    memset(&method_struct, 0, sizeof(method_struct));
    http_get_method(fd, &method_struct);
    for (;;) {
        char temp_buf[MAX_LEN];
        memset(temp_buf, 0, sizeof(temp_buf));
        int n = http_read_line(fd, temp_buf);
        memmove(ptr, temp_buf, n);
        temp_buf[n] = 0;
        strcat(http_header, temp_buf);
        ptr += n;
        line_size += n;
        if (memcmp(HTTP_BOUNDARY, temp_buf, 2) == 0) {
            break;
        }
    }
    if (line_size > 0) {
        buf[line_size] = 0;
    }
    return line_size;
}

char *http_get_remote_host(char *http_header) {
    char line[MAX_LEN];
    char *remote_host = (char *)malloc(100);
    char header[MAX_LEN];
    char header_value[MAX_LEN];
    char *header_str = "host: ";
    int i;
    char ch = 0;
    char *local_header = http_header;
    memset(line, 0, sizeof(line));
    for (;;) {
        while (ch != '\n') {
            memset(&ch, local_header[i], 1);
            strncat(line, &ch, 1);
            i++;
        }
        sscanf(line, "%s %s\r\n", header, header_value);
        string_to_loader(header);
        // 如果匹配到 host 头，则 break
        if(strcspn(header, header_str) == 0) {
            strcpy(remote_host, header_value);
            break;
        }
        // 如果到了 http header 的结尾，则 break
        if (memcmp(HTTP_BOUNDARY, line, 2) == 0) {
            break;
        }
        memset(line, 0, sizeof(line));
        ch = 0;
        i = 0;
    }
    return remote_host;
}

int http_get_method(unsigned int fd, method_t *p) {
    char temp_buf[MAX_LEN];
    int n = http_read_line(fd, temp_buf);
    char *method = (char *)malloc(MAX_LEN);
    char *url = (char *)malloc(MAX_LEN);
    char *version = (char *)malloc(MAX_LEN);
    memset(method, 0, MAX_LEN);
    memset(url, 0, MAX_LEN);
    memset(version, 0, MAX_LEN);
    if (n > 0) {
        sscanf(temp_buf, "%s %s %s\r\n", method, url, version);
        p->method = method;
        p->url = url;
        p->version = version;
    } else {
        return 1;
    }
    return 0;
}

int http_read_line(unsigned int fd, char buf[]) {
    char local_buf[MAX_LEN];
    memset(local_buf, 0, sizeof(local_buf));
    char *local_bug_ptr = local_buf;
    char ch = 0;
    int count, sum = 0;
    while (ch != '\n') {
        count = read(fd, &ch, 1);
        if (count == 0) {
            // EOF
            return 0;
        }
        if (count < 0) {
            // error
            return -1;
        }
        memmove(local_bug_ptr, &ch, count);
        local_bug_ptr += count;
        sum += count;
    }
    memcpy(buf, local_buf, sum);
    return sum;
}

void string_to_loader(char *str) {
    char *local_ptr = str;
    for(int i = 0; local_ptr[i]; i++){
        local_ptr[i] = tolower(local_ptr[i]);
    }
}