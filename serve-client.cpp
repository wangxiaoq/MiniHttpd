#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#define MAX_BUF_LEN 4096
#define WWW_ROOT "www"

static int get_line(int fd, char *buf, int len)
{
    char ch;
    int i = 0;

    while ((recv(fd, &ch, 1, 0) > 0) && (ch != '\n')) {
        buf[i++] = ch;
    }

    return i;
}

static void clear_header(int sock_fd)
{
    char line[MAX_BUF_LEN] = {0};
    int ret = 0;

    do {
        memset(line, 0, MAX_BUF_LEN);
        ret = get_line(sock_fd, line, MAX_BUF_LEN);
    } while (ret != 1 && strcmp(line, "\r"));
}

static void not_found404(int sock_fd)
{
    const char *status_line = "HTTP/1.0 404 NotFound\r\n";
    const char *type_line = "Content-Type:text/html;charset=ISO-8859-1\r\n";
    const char *blank_line = "\r\n";
    char not_found_html[MAX_BUF_LEN] = {0};
    int fd = -1;
    struct stat buf;

    strcpy(not_found_html, WWW_ROOT);
    strcat(not_found_html, "/404.html");

    if (stat(not_found_html, &buf) < 0) {
        return ;
    }

    fd = open(not_found_html, O_RDONLY);
    if (fd < 0) {
        return ;
    }

    if (send(sock_fd, status_line, strlen(status_line), 0) < 0) {
        return ;
    }

    if (send(sock_fd, type_line, strlen(type_line), 0) < 0) {
        return ;
    }

    if (send(sock_fd, blank_line, strlen(blank_line), 0) < 0) {
        return ;
    }

    if (sendfile(sock_fd, fd, 0, buf.st_size) < 0) {
        return ;
    }

    close(fd);
}

static void echo_www(int sock_fd, char *path)
{
    const char *status_line = "HTTP/1.0 200 OK\r\n";
    const char *type_line = "Content-Type:text/html;charset=ISO-8859-1\r\n";
    const char *blank_line = "\r\n";
    struct stat buf;
    int fd = -1;

    if (stat(path, &buf) < 0) {
        not_found404(sock_fd);
        return ;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return ;
    }

    if (send(sock_fd, status_line, strlen(status_line), 0) < 0) {
        return ;
    }

    if (send(sock_fd, type_line, strlen(type_line), 0) < 0) {
        return ;
    }

    if (send(sock_fd, blank_line, strlen(blank_line), 0) < 0) {
        return ;
    }

    if (sendfile(sock_fd, fd, 0, buf.st_size) < 0) {
        return ;
    }

    close(fd);
}

static int exec_cgi(int sock_fd, char *method, char *path, char *parameter)
{
    int ret = 0;
    pid_t pid = 0;
    int in[2], out[2];
    int len = -1;
    char line[MAX_BUF_LEN] = {0};
    char ch;
    int i = 0;
    char len_str[MAX_BUF_LEN] = {0};
    const char *status_line = "HTTP/1.0 200 OK\r\n";
    const char *content_type = "Content-Type:text/html;charset=ISO-8859-1\r\n";
    const char *blank_line = "\r\n";
    int status;

    send(sock_fd, status_line, strlen(status_line), 0);
    send(sock_fd, content_type, strlen(content_type), 0);
    send(sock_fd, blank_line, strlen(blank_line), 0);

    ret = pipe(in);
    if (ret < 0) {
        return ret;
    }
    ret = pipe(out);
    if (ret < 0) {
        return ret;
    }

    if (!strcasecmp(method, "GET")) {
        clear_header(sock_fd);
    } else {
        do {
            memset(line, 0, MAX_BUF_LEN);
            ret = get_line(sock_fd, line, MAX_BUF_LEN);
            if (!strncasecmp("Content-Length:", line, 15)) {
                sscanf(line, "Content-Length: %d", &len);
                sprintf(len_str, "%d", len);
            }
        } while (ret > 0 && strcmp(line, "\r"));
        if (len < 0) {
            return -1;
        }
    }

    pid = fork();
    if (pid < 0) {
        return pid;
    } else if (pid == 0) {
        close(in[1]);
        close(out[0]);
        if (dup2(in[0], 0) < 0 || dup2(out[1], 1) < 0) {
            return -1;
        }
        if (setenv("METHOD", method, 1) ||
            setenv("URL", path, 1)) {
            exit(-1);
        }

        if (parameter && setenv("QUERY-STRING", parameter, 1)) {
            exit(-1);
        }
        if (!strcasecmp("POST", method) && setenv("CONTENT-LENGTH", len_str, 1)) {
            exit(-1);
        }

        ret = execl(path, path, NULL);
        exit(ret);
    } else {
        close(in[0]);
        close(out[1]);

        if (!strcasecmp("POST", method)) {
            do {
                ret = recv(sock_fd, &ch, 1, 0);
                write(in[1], &ch, 1);
                i++;
            } while (ret > 0 && i < len);
        }

        do {
            ret = read(out[0], line, MAX_BUF_LEN);
            if (ret > 0)
                send(sock_fd, line, ret, 0);
        } while (ret > 0);

        waitpid(pid, &status, 0);
    }

    return 0;
}

/* serve the request from clients */
void* serve_client_request(void *arg)
{
    int sock_fd = (int)(long)arg;
    char buf[MAX_BUF_LEN] = {0};
    char method[MAX_BUF_LEN] = {0};
    char path[MAX_BUF_LEN] = {0};
    int i = 0, j = 0, k = 0;
    char *get_parameter = NULL;
    bool use_cgi = false;

    pthread_detach(pthread_self());

    if (get_line(sock_fd, buf, MAX_BUF_LEN) <= 0) {
        return NULL;
    }

    /* get the method */
    while (buf[i] != ' ') {
        method[j++] = buf[i++];
    }

    /* skip the space & get the request path */
    while (buf[i] == ' ') i++;

    strcpy(path, WWW_ROOT);
    j = strlen(path);
    k = j;
    while (buf[i] != ' ') {
        path[j++] = buf[i++];
    }

    if (path[strlen(path)-1] == '/') {
        strcat(path, "index.html");
    }

    /* check wether the parameter exist */
    if (!strcasecmp(method, "GET")) {
        get_parameter = &path[k];
        while (*get_parameter != '?' && *get_parameter != '\0') {
            get_parameter++;
        }
        if (*get_parameter == '?') {
            use_cgi = true;
            *get_parameter = '\0';
            get_parameter++;
        }
    }

    /* GET method */
    if (!strcasecmp(method, "GET") && !use_cgi) {
        clear_header(sock_fd);
        echo_www(sock_fd, path);
    } else {
        exec_cgi(sock_fd, method, path, get_parameter);
    }

    close(sock_fd);

    return NULL;
}
