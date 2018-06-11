#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define HTTP_PORT 80
#define MAX_BUF_LEN 4096
#define SERVER_STRING "Server: MiniHttpd"

/* initialize as a daemon */

/* 
 * create a TCP server 
 *
 * @type: socket type
 * @addr: server address
 * @len: sockaddr's length
 *
 * return value: > 0: the server's socket
 *               < 0: error
 * */
static int create_tcp_server(int type, struct sockaddr *addr, socklen_t len, int qlen)
{
    int ret = 0;
    int sockfd = socket(addr->sa_family, type, 0);
    if (sockfd < 0) {
        return -1;
    }

    ret = bind(sockfd, addr, len);
    if (ret < 0) {
        return -1;
    }

    ret = listen(sockfd, qlen);
    if (ret < 0) {
        return -1;
    }

    return sockfd;
}

/* serve the request from clients */
static int serve_client_request(int csockfd, char* request, int len)
{
    int ret = 0;
    char buf[MAX_BUF_LEN] = {0};
    /* copy the status line */
    strcpy(buf, "HTTP/1.1 200 OK\r\n");
    /* short connection */
    strcat(buf, "Connection: close\r\n");
    strcat(buf, "Content-Length: 18\r\n");
    strcat(buf, "Content-Type: text/html\r\n");
    strcat(buf, "\r\n");
    strcat(buf, "<html>hello</html>");
    ret = send(csockfd, buf, strlen(buf), 0);

    if (ret < 0) {
        return -1;
    }

    return 0;
}

/* 
 * accept a request and serve
 *
 * @sockfd: the server's socket fd
 *
 * */
static void accept_request_and_serve(int sockfd)
{
    int client_fd = 0;
    char buf[MAX_BUF_LEN] = {0};

    while (1) {
        client_fd = accept(sockfd, NULL, NULL);
        if (client_fd < 0) {
            continue;
        }
        recv(client_fd, buf, MAX_BUF_LEN, 0);
        serve_client_request(client_fd, NULL, 0);
        close(client_fd);
    }
}

int main(int argc, char *argv[])
{
    int server_fd = 0;
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(HTTP_PORT),
        .sin_addr = INADDR_ANY
    };

    server_fd = create_tcp_server(SOCK_STREAM, (struct sockaddr *)&addr, sizeof(addr), 5);
    if (server_fd < 0) {
        return -1;
    }

    accept_request_and_serve(server_fd);

    return 0;
}