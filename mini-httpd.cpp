#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>

const unsigned int HTTP_PORT = 80;
#define MAX_BUF_LEN 4096
#define SERVER_STRING "Server: MiniHttpd"

class HTTPDaemon {
private:
    std::string ip_addr_str;
    unsigned int ip_addr;
    const unsigned int port;
    const std::string server_string;
    int daemonize();    /* become a daemon */
    int create_tcp_server(int type, struct sockaddr *addr, socklen_t len, int qlen);
    int serve_client_request(int csockfd, char* request, int len);
    void accept_request_and_serve(int sockfd);

public:
    HTTPDaemon(const unsigned int p, const std::string &s, const std::string &ip): port(p), server_string(s), ip_addr_str(ip) 
    {
        ip_addr = inet_addr(ip.c_str());
    }
    int start_httpd();
};

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
int HTTPDaemon::serve_client_request(int csockfd, char* request, int len)
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
void HTTPDaemon::accept_request_and_serve(int sockfd)
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

int HTTPDaemon::start_httpd(void)
{
    int server_fd = 0;
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };
    addr.sin_addr.s_addr = ip_addr;

    server_fd = create_tcp_server(SOCK_STREAM, (struct sockaddr *)&addr, sizeof(addr), 5);
    if (server_fd < 0) {
        return -1;
    }

    accept_request_and_serve(server_fd);

    return 0;
}

int main(int argc, char *argv[], char* envp[])
{
    HTTPDaemon httpd(HTTP_PORT, SERVER_STRING, "127.0.0.1");

    httpd.start_httpd();

    return 0;
}
