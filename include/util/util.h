#ifndef WEBSERVER_UTIL_H_
#define WEBSERVER_UTIL_H_

#include <cstdlib>
#include <string>
#include <map>
#include <memory>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <cstring>
#include <unistd.h>
#include <signal.h>

#include "connection_pool.h"
#include "http_conn.h"

using namespace std;


//设置文件描述符为非阻塞
int setnonblocking(int fd);

void addfd(int epollfd, int fd, bool one_shot);

void removefd(int epollfd, int fd);

void modfd(int epollfd, int fd, int ev);

int open_listenfd(int port);

// void addsig(int sig, void(handler)(int), bool restart = true);

void reset_oneshot(int epollfd, int fd);

// void sig_handler(int sig);

void close_http_conn_cb_func(shared_ptr<http_conn> user);

map<string, string> parse_form(string str);

bool login_u(string username, string passwd);

bool register_u(string username, string passwd);

void addsig(int sig, void(handler)(int), bool restart = true);
class Sig
{
public:
    static int pipefd[2];
    static int epollfd;
    static void init(int epollfd_)
    {
        epollfd = epollfd_;
        int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
        if(ret==-1){
            printf("create socketpait faild");
            abort();
        }
        setnonblocking(pipefd[0]);
        addfd(epollfd, pipefd[0], false);
    }
    static bool AddSignal()
    {
        //添加信号处理函数
        addsig(SIGPIPE, SIG_IGN);

        addsig(SIGALRM, sig_handler, true);
        addsig(SIGTERM, sig_handler, true);
        addsig(SIGINT,sig_handler,true);
        return true;
    }
    static void sig_handler(int sig)
    {
        //为保证函数的可重入性，保留原来的errno
        int save_errno = errno;
        int msg = sig;
        send(pipefd[1], (char *)&msg, 1, 0);
        errno = save_errno;
    }
};

#endif //WEBSERVER_UTIL_H_