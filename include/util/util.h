#pragma once

#include <stdlib.h>
#include <string>
#include"http_conn.h"
#include "timer.h"

int setnonblocking(int fd);
void addfd(int epollfd,int fd,bool one_shot);
void removefd(int epollfd, int fd);
void modfd(int epollfd, int fd, int ev);
int open_listenfd(int port);

class Util{
public:
    static int pipefd[2];
    static int epollfd;
    //定时器相关
    static TimeHeap *time_heap;
    static void init(int epollfd_,TimeHeap *time_heap_){
        //定时器
        time_heap = time_heap_;
        epollfd = epollfd_;
        int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
        assert(ret != -1);
        setnonblocking(Util::pipefd[1]);
        addfd(epollfd, Util::pipefd[0], false);
    }
private:
    Util();
    ~Util();
};
void addsig(int sig, void(handler)(int), bool restart=true);
void reset_oneshot(int epollfd, int fd);
void sig_handler(int sig);
void close_http_conn_cb_func(http_conn *user);