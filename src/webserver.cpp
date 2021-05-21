#include "webserver.h"
WebServer::WebServer()
{
    //http_conn类对象
    users = new http_conn[MAX_FD];

    //root文件夹路径
    char server_path[200];
    getcwd(server_path, 200);
    char root[6] = "/root";
    root_dir = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(root_dir, server_path);
    strcat(root_dir, root);
}

WebServer::~WebServer()
{
    close(epollfd);
    close(listenfd);
    delete[] users;
    delete[] time_heap;
    delete thread_pool;
    free(root_dir);
}

void WebServer::init(Config config)
{
    port = config.port;
    sql_user = config.sql_user;
    sql_passwd = config.sql_passwd;
    sql_db_name = config.sql_db_name;
    sql_num = config.sql_num;
    thread_num = config.thread_num;
    opt_linger = config.opt_linger;
    close_log = config.close_log;
    timeout=false;
    stop_server=false;
    //日志初始化
    Log::get_instance()->init("./ServerLog", close_log, 2000, 800000);

    //数据库连接池初始化
    ConnectionPool::GetInstance()->init("localhost", sql_user, sql_passwd, sql_db_name, 3306, sql_num);

    //线程池初始化
    thread_pool = new threadpool<http_conn>(thread_num,10000); //TODO:max_request

    //初始化事件监听描述符 epoll 监听描述符 listenfd
    initIO();
}


void WebServer::initIO()
{
    listenfd = open_listenfd(port);

    epollfd = epoll_create(5);
    assert(epollfd != -1);

    //定时器
    time_heap = new TimeHeap(MAX_FD);

    Util::init(epollfd,time_heap);
    http_conn::m_epollfd = epollfd;

    // addfd(epollfd, listenfd, false);//TODO:ET mode
    epoll_event event;
    event.data.fd = listenfd;

    event.events = EPOLLIN | EPOLLRDHUP;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event);
    setnonblocking(listenfd);

    addsig(SIGPIPE, SIG_IGN);
    addsig(SIGALRM, sig_handler, true);
    addsig(SIGTERM, sig_handler, true);

    alarm(TIMESLOT);
}

void WebServer::initHttpConn(int connfd, struct sockaddr_in client_address)
{
    users[connfd].init(connfd, client_address,root_dir);

    if(opt_linger){
        struct linger linger_={1,1};
        setsockopt(connfd,SOL_SOCKET,SO_LINGER,&linger_,sizeof(linger_));
    }
    //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
    heap_timer *timer = new heap_timer(3 * TIMESLOT);
    timer->user_data = &users[connfd];
    timer->cb_func = close_http_conn_cb_func;
    
    users[connfd].timer = timer;
    time_heap->add_timer(timer);
}

//若有数据传输，则将定时器往后延迟3个单位
//并对新的定时器在链表上的位置进行调整
void WebServer::adjustTimer(heap_timer *timer)
{
    time_heap->adjustTimer(timer,3 * TIMESLOT);

    LOG_INFO("%s", "adjust timer once");
}


bool WebServer::acceptClient()
{
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
    if (connfd < 0)
    {
        LOG_ERROR("%s:errno is:%d", "accept error", errno);
        return false;
    }
    if (http_conn::m_user_count >= MAX_FD)
    {
        char info[] = "Internal server busy";
        send(connfd, info, strlen(info), 0);
        close(connfd);
        LOG_ERROR("%s", info);
        return false;
    }
    char buf[20];
    // printf("accept client:%d from %s:%d\n",connfd,inet_ntop(AF_INET,&client_address.sin_addr,buf,INET_ADDRSTRLEN),ntohs(client_address.sin_port));
    LOG_INFO("accept client from %s:%d",inet_ntop(AF_INET,&client_address.sin_addr,buf,INET_ADDRSTRLEN),ntohs(client_address.sin_port));
    initHttpConn(connfd, client_address);
    return true;
}

bool WebServer::dealSignal()
{
    int ret = 0;
    char signals[1024];
    ret = recv(Util::pipefd[0], signals, sizeof(signals), 0);
    if (ret == -1)
    {
        return false;
    }
    else if (ret == 0)
    {
        return false;
    }
    else
    {
        for (int i = 0; i < ret; ++i)
        {
            switch (signals[i])
            {
            case SIGALRM:
            {
                timeout = true;
                break;
            }
            case SIGTERM:
            {
                stop_server = true;
                break;
            }
            default:{
                LOG_INFO("%s","get other signal ,deal with default");
            }
            }
        }
    }
    return true;
}

void WebServer::dealRead(int sockfd)
{
    heap_timer *timer = users[sockfd].timer;
    if (timer)
    {
        adjustTimer(timer);
    }
    //若监测到读事件，将该事件放入请求队列
    users[sockfd].setRead();
    thread_pool->append(users + sockfd);//第二个参数为
}

void WebServer::dealWrite(int sockfd)
{
    heap_timer *timer = users[sockfd].timer;
    if (timer)
    {
        adjustTimer(timer);
    }
    //若监测到写事件，将该事件放入请求队列
    users[sockfd].setWrite();
    thread_pool->append(users + sockfd);
}

void WebServer::run()
{
    timeout = false;
    stop_server = false;

    while (!stop_server)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR)
        {
            LOG_ERROR("%s", "epoll failure");
            break;
        }
        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            //处理新到的客户连接
            if (sockfd == listenfd)
            {
                acceptClient();
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                //如有异常 关闭客户端链接
                users[sockfd].close_conn();
            }
            //处理信号
            else if ((sockfd == Util::pipefd[0]) && (events[i].events & EPOLLIN))
            {
                bool flag = dealSignal();
                if (false == flag)
                    LOG_ERROR("%s", "deal signal failure");
            }
            //处理客户连接上接收到的数据
            else if (events[i].events & EPOLLIN)
            {
                dealRead(sockfd);
            }
            else if (events[i].events & EPOLLOUT)
            {
                dealWrite(sockfd);
            }
        }
        if (timeout)
        {
            time_heap->tick();
            LOG_INFO("timer tick time_heap.size()=%d",time_heap->size());
            alarm(TIMESLOT);
            timeout = false;
        }
    }
}