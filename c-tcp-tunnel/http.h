#ifndef WHEELS_HTTP_H
#define WHEELS_HTTP_H

#define MAX_LEN 8192
#define HTTP_BOUNDARY "\r\n"
char http_header[MAX_LEN];

typedef struct {
    char *method;
    char *url;
    char *version;
} method_t;

int http_read_line(unsigned int fd, char buf[]);

int http_get_header(unsigned int fd, char buf[]);

int http_get_method(unsigned int fd, method_t *p);

char *http_get_remote_host(char *http_header);

#endif //WHEELS_HTTP_H
