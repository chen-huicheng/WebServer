#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <unistd.h>
#include <cerrno>
#include <fcntl.h>
#include <cstdlib>
#include <cassert>
#include <sys/epoll.h>

#include <memory>
#include <vector>

#include "threadpool.h"
#include "http_conn.h"
#include "config.h"
#include "timer.h"
#include "util.h"
#include "logger.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时单位

class WebServer
{
public:
    WebServer();
    ~WebServer();
    void init(Config config);
    void loop();

private:
    void adjustTimer(shared_ptr<heap_timer> timer);
    void initHttpConn(int connfd, struct sockaddr_in client_address);
    bool acceptClient();
    bool dealTimer();
    bool dealSignal();
    void dealRead(int sockfd);
    void dealWrite(int sockfd);

    void initIO();
    //基础
    int port;
    string root_dir;
    int close_log;

    vector<shared_ptr<http_conn>> users;


    //线程池相关
    shared_ptr<threadpool<http_conn>> thread_pool;
    int thread_num;
    int max_request;

    //epoll_event相关
    int epollfd;
    epoll_event events[MAX_EVENT_NUMBER];

    int listenfd;
    int opt_linger;

    //定时器
    shared_ptr<TimeHeap> time_heap;

    //信号处理相关
    bool timeout = false;
    bool stop_server = false;
};
#endif //WEBSERVER_H_