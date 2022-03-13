#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <tls.h>
#include <sys/select.h>
#include <errno.h>
#include "llhttp.h"
#include "common.h"
#include "httpheaderlist.h"
#include "deps/cJSON.h"
#include "deps/base64.h"
#include "utils.h"
#include "config.h"

#define SERVER "main"

// 16 位无符号整型
uint16_t PORT;

typedef struct tls tls_t;
typedef struct tls_config tls_config_t;

// 服务器的 socket fd;
unsigned int server_sock_fd;
// 客户端 socket fd;
unsigned int client_sock_fd;
// 远程服务器的 socket fd
unsigned int remote_sock_fd;

uint16_t remote_server_port = 443;

ssize_t tls_read_n(tls_t *ptr, char *buf, ssize_t len, llhttp_t *);

ssize_t tls_write_n(tls_t *ptr, char *buf, ssize_t len);
void close_all();
void free_all();

tls_config_t *tls_config_ptr;
tls_t *c_context = NULL;
tls_t *tls_remote_server_context_ptr = NULL, *tls_server_context_ptr = NULL;
typedef List header_list_t;
typedef Item header_item_t;
typedef Node header_node_t;

int on_message_complete(llhttp_t* parser);
int on_header_field(llhttp_t* parser, const char* at, size_t length);
int on_header_value(llhttp_t* parser, const char* at, size_t length);
int on_headers_complete(llhttp_t* parser);
int on_body(llhttp_t* parser, const char* at, size_t length);
llhttp_errno_t pass_http_entity_to_llhttp(llhttp_t* parser, char *buf);
int config_tls_server_context(tls_t *context, tls_config_t *config, uint32_t *protocols);
void write_connection_established (tls_t * ctx);
void write_connection_fail (tls_t * ctx);
void write_success(tls_t * ctx);
// 根据域名获取服务器 host 并建立 socket 连接，写到第二个参数指向的地址中
int get_remote_server_socket_fd(const char *domain_name, unsigned int *socket_fd);
void init_llhttp_settings(llhttp_settings_t *);
int use_tunnel(void);
int fd(void);
void show_headers(header_item_t item);
header_item_t *find_empty_http_header_item(const header_list_t *plist, bool (*pfun)(header_item_t item));
bool find_empty_http_header_item_handler(header_item_t item);
header_item_t *find_http_header_item_by_field(const header_list_t *plist, const char *field);

header_list_t headers_list;

llhttp_t parser;
llhttp_settings_t settings;

uint32_t protocols = TLS_PROTOCOLS_DEFAULT;

int main(int argc, char **argv) {
    if (argc < 2) {
        puts("missing port, exit.(./main <port>)");
        exit(0);
    }
    sscanf(argv[1], "%hu", &PORT);

    int conf_ret = config_init();
    if(conf_ret < 0) {
        exit(1);
    }

    tls_init();
    tls_config_ptr = tls_config_new();
    tls_server_context_ptr = tls_server();
//    tls_remote_server_context_ptr = tls_client();

    if(config_tls_server_context(tls_server_context_ptr, tls_config_ptr, &protocols) < 0) {
        exit(1);
    }

    /* socket operation start */
    struct sockaddr_in server_addr;
    int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock_fd == -1) {
        puts("socket error");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt = 1;
//    setsockopt(server_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, 4);
    bind(server_sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    listen(server_sock_fd, 5);
    printf("listening port: %d\n", PORT);

    // init header list --- start
    http_header_initializeList(&headers_list);
    // init header list --- end

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    printf("waiting for request\n");
    client_sock_fd = accept(server_sock_fd, (struct sockaddr *) &client_addr, &client_addr_len);
    if(client_sock_fd > 0) {
        printf("connect from client\n");
    } else {
        puts("accept error");
        exit(1);
    }
    /* socket operation end */
    if(tls_accept_socket(tls_server_context_ptr, &c_context, client_sock_fd) < 0){
        printf("tls_accept_socket error: %s\n", tls_error(tls_server_context_ptr));
        exit(1);
    }

    char read_buf[8192];
    char read_buf2[8192];
    memset(read_buf, 0, sizeof(read_buf));
    memset(read_buf2, 0, sizeof(read_buf2));

    // llhttp settings start
    /* Initialize user callbacks and settings */
    init_llhttp_settings(&settings);
    /* Initialize the parser in HTTP_BOTH mode, meaning that it will select between
     * HTTP_REQUEST and HTTP_RESPONSE parsing automatically while reading the first
     * input.
     */
    llhttp_init(&parser, HTTP_BOTH, &settings);
    // llhttp settings end

    // 读取请求
    tls_read_n(c_context, read_buf, sizeof(read_buf), &parser);

    printf("read_buf: \n%s\n", read_buf);
    llhttp_errno_t llhttp_err_no;
    if((llhttp_err_no = llhttp_get_errno(&parser)) == HPE_PAUSED_UPGRADE) {
        printf("HPE_PAUSED_UPGRADE\n");
        char server_name[100];
        get_remote_server_socket_fd("www.baidu.com", &remote_sock_fd);
        write_connection_established(c_context);
        // 连接远程服务器 end
        memset(read_buf, 0, sizeof(read_buf));
        use_tunnel();
    } else {
        write_success(c_context);
    }

    return 0;
}

ssize_t tls_write_n(tls_t *ptr, char *buf, ssize_t len) {
    char *local_buf = buf;
    ssize_t left = len;
    while (left > 0) {
        ssize_t count = tls_write(ptr, local_buf, left);
        if (count == TLS_WANT_POLLIN || count == TLS_WANT_POLLOUT) {
            continue;
        }
        if (count == 0) { // EOF
            break;
        }
        if (count < 0) { // Error
            printf("tls_write_n error: %s\n", tls_error(tls_server_context_ptr));
            break;
        }
        local_buf += count;
        left -= count;
    }
    return len - left;
}

ssize_t tls_read_n(tls_t *ptr, char *buf, ssize_t len, llhttp_t *parser) {
    char *local_buf = buf;
    ssize_t left = len;
    llhttp_errno_t http_errno;
    while (left > 0) {
        ssize_t count = tls_read(ptr, local_buf, left);
        if (count == TLS_WANT_POLLIN || count == TLS_WANT_POLLOUT) {
            continue;
        }
        if (count == 0) { // EOF
            break;
        }
        if (count == -1) { // Error
            printf("tls_read_n() error\n");
            return count;
        }
        local_buf += count;
        left -= count;
        http_errno = pass_http_entity_to_llhttp(parser, buf);
        if(http_errno == HPE_OK || http_errno == HPE_PAUSED_UPGRADE) {
            break;
        }
    }
    return len - left;
}

llhttp_errno_t pass_http_entity_to_llhttp(llhttp_t* parser, char *buf) {
    char *local_buf = buf;
    enum llhttp_errno err = llhttp_execute(parser, local_buf, strlen(local_buf));
    if (err == HPE_OK) {
        /* Successfully parsed! */
        if(parser->finish != HTTP_FINISH_SAFE) {
            llhttp_reset(parser);
        }
    } else if(err == HPE_PAUSED_UPGRADE) {
        fprintf(stderr, "HPE_PAUSED_UPGRADE Parse error: %s %s\n", llhttp_errno_name(err), parser->reason);
        return err;
    } else {
        fprintf(stderr, "Parse error: %s %s\n", llhttp_errno_name(err), parser->reason);
        return err;
    }
    return err;
}

int on_header_field(llhttp_t* parser, const char* at, size_t length)
{
    char header_field[MAX_LEN];
    strncpy(header_field, at, length);
    header_field[length] = '\0';
    printf("head field: %s\n", header_field);

    header_item_t header_item;
//    char *lower = utils_str_to_lower_case(header_field);
    strcpy(header_item.header, header_field);
    http_header_addItem(header_item, &headers_list);
    return 0;
}

int on_header_value(llhttp_t* parser, const char* at, size_t length)
{
    char header_value[MAX_LEN];
    strncpy(header_value, at, length);
    header_value[length] = '\0';
    printf("head value: %s\n", header_value);
    // 查找第一个 header 但 value 为空字符串的 header_item
    header_item_t *item = find_empty_http_header_item(&headers_list, find_empty_http_header_item_handler);
    if(item != NULL) {
        if(strcmp(item->header, "proxy-authorization") == 0) {
            strcpy(item->value, header_value);
        } else {
//            char *lower = utils_str_to_lower_case(header_value);
            // 赋值 value
            strcpy(item->value, header_value);
        }
    }
    return 0;
}

int on_headers_complete(llhttp_t* parser)
{
//    printf("on_headers_complete, major: %d, minor: %d, keep-alive: %d, upgrade: %d\n", parser->http_major, parser->http_minor, llhttp_should_keep_alive(parser), parser->upgrade);
//    printf("content length: %llu\n", parser->content_length);
    // show_headers
    if(http_header_listIsEmpty(&headers_list) == false) {
        printf("on_headers_complete()\n");
//        http_header_traverse(&headers_list, show_headers);
        header_item_t *item = find_http_header_item_by_field(&headers_list, "host");
        header_item_t *proxy_auth_item = find_http_header_item_by_field(&headers_list, "proxy-authorization");
//        show_headers(*item);
//        show_headers(*proxy_auth_item);
        char *proxy_auth_decode;
        char proxy_auth_str[MAX_LEN];
        memset(proxy_auth_str, 0, sizeof(proxy_auth_str));
        if(proxy_auth_item != NULL) {
//            printf("proxy_auth_item value: %s\n", proxy_auth_item->value);
            sscanf(proxy_auth_item->value, "Basic %s", proxy_auth_str);
//            printf("proxy_auth_str: %s\n", proxy_auth_str);
            base64_decode(proxy_auth_str, &proxy_auth_decode);
//            printf("base64 decode: %s\n", proxy_auth_decode);
        } else {
            puts("proxy_auth_item->value is empty");
        }
        printf("\n");
    }
    return 0;
}

int on_body(llhttp_t* parser, const char* at, size_t length)
{
    char body[MAX_LEN];
    strncpy(body, at, length);
    body[length] = '\0';
    printf("on_body length %zu, content %s\n", length, body);
    return 0;
}

int on_message_complete(llhttp_t* parser)
{
    printf("on_message_complete\n");
    return 0;
}

void close_all() {
    tls_close(tls_server_context_ptr);
    close(server_sock_fd);
    close(client_sock_fd);
    close(remote_sock_fd);
}
void free_all() {
    tls_free(tls_server_context_ptr);
    tls_config_free(tls_config_ptr);
}

int config_tls_server_context(tls_t *context, tls_config_t *config, uint32_t *protocols) {
    tls_t *local_context = context;
    tls_config_t *local_config = config;
    uint32_t *local_protocols = protocols;

    if(tls_config_parse_protocols(local_protocols, "secure") < 0) {
        printf("tls_config_parse_protocols error\n");
        return -1;
    }
    tls_config_set_protocols(local_config, *local_protocols);
    if(tls_config_set_ciphers(local_config, "secure") < 0) {
        printf("tls_config_set_ciphers error\n");
        return -1;
    }
//    printf("config_key_file_path %s\n", config_key_file_path);
//    printf("config_crt_file_path %s\n", config_crt_file_path);
    if(tls_config_set_key_file(local_config, config_key_file_path) < 0) {
        printf("tls_config_set_key_file error\n");
        return -1;
    }
    if(tls_config_set_cert_file(local_config, config_crt_file_path) < 0) {
        printf("tls_config_set_cert_file error\n");
        return -1;
    }
    if(tls_configure(local_context, local_config) < 0) {
        printf("tls_configure error: %s\n", tls_error(local_context));
        return -1;
    }
    return 0;
}

void write_connection_established(tls_t *ctx) {
    char buf[MAX_LEN] = "HTTP/1.1 200 OK\r\n";
    strcat(buf, "Proxy-Agent: main");
    strcat(buf, "\r\n");
    strcat(buf, "Transfer-Encoding: chunked");
    strcat(buf, "\r\n");
    strcat(buf, "\r\n");
    tls_write_n(ctx, buf, strlen(buf));
}

void write_connection_fail(tls_t *ctx) {
    char buf[MAX_LEN] = "HTTP/1.1 504 Bad Gateway\r\n";
    strcat(buf, "Proxy-Agent: main");
    strcat(buf, "\r\n");
    strcat(buf, "Transfer-Encoding: chunked");
    strcat(buf, "\r\n");
    strcat(buf, "\r\n");
    tls_write_n(ctx, buf, strlen(buf));
}

void write_success(tls_t *ctx) {
    char buf[MAX_LEN] = "HTTP/1.1 200 OK";
    strcat(buf, "\r\n");
    strcat(buf, "Accept-Ranges: bytes");
    strcat(buf, "\r\n");
    strcat(buf, "Content-Length: 32");
    strcat(buf, "\r\n");
    strcat(buf, "Server: ");
    strcat(buf, SERVER);
    strcat(buf, "\r\n");
    strcat(buf, "Content-Type: text/html; charset=utf-8");
    strcat(buf, "\r\n");
    strcat(buf, "\r\n");
    strcat(buf, "<html><body>haha</body></html>");
    strcat(buf, "\r\n");
    tls_write_n(ctx, buf, strlen(buf));
}

int get_remote_server_socket_fd(const char *domain_name, unsigned int *socket_fd) {
    struct hostent *host_ent;
    host_ent = gethostbyname(domain_name);
    if (host_ent == NULL) {
        perror("connect_remote_server() gethostbyname error ");
        return 1;
    }
    struct sockaddr_in remote_server_addr;
    memset(&remote_server_addr, 0, sizeof(remote_server_addr));
    remote_server_addr.sin_family = AF_INET;
    remote_server_addr.sin_port = htons(remote_server_port);
    memcpy(&remote_server_addr.sin_addr.s_addr, host_ent->h_addr_list[0], host_ent->h_length);
    unsigned int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("connecting remote server...\n");
    int conn_status = connect(sock_fd, (struct sockaddr *) &remote_server_addr, sizeof(remote_server_addr));
    if(conn_status < 0) {
        printf("connecting remote server fail\n");
        return conn_status;
    } else {
        printf("connect remote server success\n");
        *socket_fd = sock_fd;
    }
    return 0;
}

void init_llhttp_settings(llhttp_settings_t *settings){
    llhttp_settings_init(settings);
    settings->on_message_complete = on_message_complete;
    settings->on_header_field = on_header_field;
    settings->on_header_value = on_header_value;
    settings->on_headers_complete = on_headers_complete;
    settings->on_body = on_body;
};

int use_tunnel(void) {
    fd_set io;
    char buffer[BUFFER_SIZE];
    
    for (;;) {
        FD_ZERO(&io);
        FD_SET(client_sock_fd, &io);
        FD_SET(remote_sock_fd, &io);
        memset(buffer, 0, sizeof(buffer));

        if (select(fd(), &io, NULL, NULL, NULL) < 0) {
            perror("use_tunnel: select()");
            break;
        }

        if (FD_ISSET(client_sock_fd, &io)) {
            int count = tls_read(c_context, buffer, sizeof(buffer));
            printf("tls_read client count %d\n", count);
            if (count < 0) {
                perror("use_tunnel: tls_read(client_sock_fd)");
                close_all();
                free_all();
                return 1;
            }

            if (count == 0) {
                close_all();
                free_all();
                return 0;
            }

            send(remote_sock_fd, buffer, count, 0);
        }

        if (FD_ISSET(remote_sock_fd, &io)) {
            int count = recv(remote_sock_fd, buffer, sizeof(buffer), 0);
            printf("recv remote count %d\n", count);
            if (count < 0) {
                perror("use_tunnel: recv(remote_socket_fd)");
                close_all();
                free_all();
                return 1;
            }

            if (count == 0) {
                close_all();
                free_all();
                return 0;
            }

            tls_write(c_context, buffer, count);
        }
    }

    return 0;
}

int fd(void) {
    unsigned int fd = client_sock_fd;
    if (fd < remote_sock_fd) {
        fd = remote_sock_fd;
    }
    return fd + 1;
}

void show_headers(header_item_t item)
{
    printf("header: %s  value: %s\n", item.header, item.value);
}

// 遍历 header_list 中的 item 并把 item 传入 pfun 作为参数，pfun 函数返回 true 则停止遍历
// 否则继续遍历，直到遍历链表的全部节点
header_item_t *find_empty_http_header_item(const header_list_t *plist, bool (*pfun)(header_item_t item)) {
    header_node_t *pnode = *plist;
    while (pnode != NULL)
    {
        if((*pfun)(pnode->item) == true){
            return &pnode->item;
        };
        pnode = pnode->next;
    }
    return NULL;
}

// 保存 header 时按照原本的大小写规则原样保存到链表中
// 查找时则转成小写来做对比
header_item_t *find_http_header_item_by_field(const header_list_t *plist, const char *field) {
    header_node_t *pnode = *plist;
    char *ptr = (char *)field;
    const char *lower_field = utils_str_to_lower_case(ptr);
    char *lower_field_in_list;
    while (pnode != NULL)
    {
        lower_field_in_list = utils_str_to_lower_case(pnode->item.header);
        if(strcmp(lower_field_in_list, lower_field) == 0){
            return &pnode->item;
        };
        pnode = pnode->next;
    }
    return NULL;
}

bool find_empty_http_header_item_handler(header_item_t item) {
    if(strlen(item.header) && strlen(item.value) == 0) {
        return true;
    }
    return false;
}
