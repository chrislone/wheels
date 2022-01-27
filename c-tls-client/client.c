#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tls.h>

size_t MAX_LEN = 8096 * 100;

typedef struct tls tls_t;
typedef struct tls_config tls_config_t;

ssize_t tls_read_n(tls_t *ptr, char *buf, ssize_t len);
ssize_t tls_write_n(tls_t *ptr, char *buf, ssize_t len);

int main(int argc, char **argv) {
    char request_format[] = "GET / HTTP/1.1\r\nHost: %s\r\nUser-Agent: curl/7.29.0\r\nAccept: */*\r\nConnection: close\r\n\r\n";
    char request[8290];
    char read_buf[MAX_LEN];
    memset(read_buf, 0, sizeof(read_buf));
    char server[100];
    if (argc < 2)
    {
        puts("format: client <domain>");
        exit(0);
    }
    sscanf(argv[1], "%s", server);
    sprintf(request, request_format, server);

    tls_init();

    tls_config_t *tls_config_ptr;
    tls_config_ptr = tls_config_new();

    tls_t *tls_context_ptr;
    tls_context_ptr = tls_client();
    int config_result = tls_configure(tls_context_ptr, tls_config_ptr);
    int connet_status = tls_connect(tls_context_ptr, server, "443");
    size_t len = sizeof(request);

    ssize_t write_count = tls_write_n(tls_context_ptr, request, len);
    if(write_count == -1) {
        exit(0);
    }

    ssize_t read_count = tls_read_n(tls_context_ptr, read_buf, sizeof(read_buf));
    if(read_count == -1) {
        exit(0);
    }

    if(read_count > 0) {
        printf("response: \n\n%s\n", read_buf);
    }
    tls_close(tls_context_ptr);
    tls_free(tls_context_ptr);
    tls_config_free(tls_config_ptr);

    return 0;
}

ssize_t tls_write_n(tls_t *ptr, char *buf, ssize_t len) {
    char *local_buf = buf;
    ssize_t left = len;
    while(left > 0) {
        ssize_t count = tls_write(ptr, local_buf, left);
        if (count == TLS_WANT_POLLIN || count == TLS_WANT_POLLOUT) {
            continue;
        }
        if(count == 0) { // EOF
            break;
        }
        if(count < 0) { // Error
            printf("tls_read_n() error\n");
            break;
        }
        local_buf += count;
        left -= count;
    }
    return len - left;
}

ssize_t tls_read_n(tls_t *ptr, char *buf, ssize_t len) {
    char *local_buf = buf;
    ssize_t left = len;
    while(left > 0) {
        ssize_t count = tls_read(ptr, local_buf, left);
        if (count == TLS_WANT_POLLIN || count == TLS_WANT_POLLOUT) {
            continue;
        }
        if(count == 0) { // EOF
            break;
        }
        if(count == -1) { // Error
            printf("tls_read_n() error\n");
            return count;
        }
        local_buf += count;
        left -= count;
    }
    return len - left;
}