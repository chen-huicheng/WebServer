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
#include "logger.h"

class http_conn;

using namespace std;


//设置文件描述符为非阻塞
int setnonblocking(int fd);
//添加文件描述符到epoll
void addfd(int epollfd, int fd, bool one_shot);
//从epoll中删除描述符
void removefd(int epollfd, int fd);
//修改描述符监听事件
void modfd(int epollfd, int fd, int ev);
//创建并监听特定端口的描述符
int open_listenfd(int port);
//重置描述法EPOLLONESHOT
void reset_oneshot(int epollfd, int fd);


void close_http_conn_cb_func(shared_ptr<http_conn> user);

map<string, string> parse_form(string str);

bool login_user(string username, string passwd);

bool register_user(string username, string passwd);

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
            LOG_ERROR("create socketpait faild\n");
            abort();
        }
        setnonblocking(pipefd[0]);
        addfd(epollfd, pipefd[0], false);
    }
    static bool AddSignal(int sig)
    {
        //添加信号处理函数
        addsig(sig,sig_handler,true);
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
void usage();
#endif //WEBSERVER_UTIL_H_