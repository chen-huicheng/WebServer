#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "threadpool.h"
#include "http_conn.h"
#include "config.h"
#include "timer.h"
#include "util.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数

class WebServer
{
public:
    WebServer();
    ~WebServer();
    void init(Config config);
    void run();
private:
    void adjustTimer(heap_timer *timer);
    void initHttpConn(int connfd, struct sockaddr_in client_address);
    bool acceptClient();
    bool dealTimer();
    bool dealSignal();
    void dealRead(int sockfd);
    void dealWrite(int sockfd);


    void initIO();
    //基础
    int port;
    char *root_dir;
    int close_log;

    http_conn *users;

    //数据库相关
    string sql_user;         //登陆数据库用户名
    string sql_passwd;     //登陆数据库密码
    string sql_db_name; //使用数据库名
    int sql_num;

    //线程池相关
    threadpool<http_conn> *thread_pool;
    int thread_num;

    //epoll_event相关
    int epollfd;
    epoll_event events[MAX_EVENT_NUMBER];

    int listenfd;
    int opt_linger;

    //定时器相关
    TimeHeap *time_heap;

    //信号处理相关
    bool timeout=false;
    bool stop_server=false;
};
#endif //WEBSERVER_H_