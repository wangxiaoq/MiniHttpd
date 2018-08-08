#ifndef MINI_HTTPD_H
#define MINI_HTTPD_H

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

#endif
