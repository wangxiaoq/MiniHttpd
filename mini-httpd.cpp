#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <errno.h>

#include "mini-httpd.h"
#include "serve-client.h"

const unsigned int HTTP_PORT = 80;
#define MAX_BUF_LEN 4096
#define SERVER_STRING "Server: MiniHttpd"

/* initialize as a daemon */
int HTTPDaemon::daemonize()
{
    return 0;
}

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
int HTTPDaemon::create_tcp_server(int type, struct sockaddr *addr, socklen_t len, int qlen)
{
    int ret = 0;
    int opt = 1;

    int sockfd = socket(addr->sa_family, type, 0);
    if (sockfd < 0) {
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

/* 
 * accept a request and serve
 *
 * @sockfd: the server's socket fd
 *
 * */
void HTTPDaemon::accept_request_and_serve()
{
    int client_fd = 0;
    char buf[MAX_BUF_LEN] = {0};
    pthread_t tid;

    while (1) {
        client_fd = accept(server_sock_fd, NULL, NULL);
        if (client_fd < 0) {
            std::cerr << "accept client error, errno = " << errno << std::endl;
            std::cerr << "server_sock_fd = " << server_sock_fd << std::endl;
            continue;
        }
//        recv(client_fd, buf, MAX_BUF_LEN, 0);
//        serve_client_request(client_fd, NULL, 0);
//        close(client_fd);
        if (pthread_create(&tid, NULL, serve_client_request, (void*)(long)client_fd) < 0) {
            continue;
        }
    }
}

int HTTPDaemon::start_httpd(void)
{
    int ret = 0;
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };
    addr.sin_addr.s_addr = ip_addr;

    ret = daemonize();
    if (ret < 0) {
        return ret;
    }

    server_sock_fd = create_tcp_server(SOCK_STREAM, (struct sockaddr *)&addr, sizeof(addr), 5);
    if (server_sock_fd < 0) {
        return -1;
    }

    accept_request_and_serve();

    return 0;
}

int main(int argc, char *argv[], char* envp[])
{
    int ret = 0;
    HTTPDaemon httpd(HTTP_PORT, SERVER_STRING, "127.0.0.1");

    ret = httpd.start_httpd();
    if (ret < 0) {
        std::cerr << "start httpd error" << std::endl;
        exit(0);
    }

    return 0;
}
