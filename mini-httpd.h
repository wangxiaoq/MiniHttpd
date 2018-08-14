#ifndef MINI_HTTPD_H
#define MINI_HTTPD_H

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class HTTPDaemon {
private:
    std::string ip_addr_str;
    unsigned int ip_addr;
    const unsigned int port;
    const std::string server_string;
    int server_sock_fd;
    int daemonize();    /* become a daemon */
    int create_tcp_server(int type, struct sockaddr *addr, socklen_t len, int qlen);
//    void *serve_client_request(void *arg);
    void accept_request_and_serve();

public:
    HTTPDaemon(const unsigned int p, const std::string &s, const std::string &ip): port(p), server_string(s), ip_addr_str(ip)
    {
        ip_addr = inet_addr(ip.c_str());
    }
    int start_httpd();
};

#endif
