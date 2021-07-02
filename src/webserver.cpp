#include "webserver.h"
WebServer::WebServer()
{
    //http_conn类对象
    users = vector<shared_ptr<http_conn>>(MAX_FD);

    for (int i = 0; i < MAX_FD; ++i)
    {
        users[i] = make_shared<http_conn>();
    }

    //root文件夹路径
    char server_path[200];
    char *root_path = getcwd(server_path, 200);
    LOG_INFO("root path:%s\n",root_path);
    char root[6] = "/root";
    root_dir.append(server_path);
    root_dir.append(root);
}

WebServer::~WebServer()
{
    close(epollfd);
    close(listenfd);
}

void WebServer::init(Config config)
{
    port = config.port;
    thread_num = config.thread_num;
    opt_linger = config.linger;
    max_request = config.max_request;
    timeout = false;
    stop_server = false;
    //线程池初始化
    thread_pool = make_shared<threadpool<http_conn>>(thread_num, max_request);

    //初始化事件监听描述符 epoll 监听描述符 listenfd
    initIO();
}

void WebServer::initIO()
{
    listenfd = open_listenfd(port);
    if (setnonblocking(listenfd) == -1)
    {

        LOG_ERROR("set socket non block failed\n");
        abort();
    }

    epollfd = epoll_create(5);
    if (-1 == epollfd)
    {
        LOG_ERROR("create epollfd failed\n");
        abort();
    }

    http_conn::m_epollfd = epollfd;

    // addfd(epollfd, listenfd, false); //ET mode
    //设置 listenfd LT触发
    epoll_event event;
    event.data.fd = listenfd;

    event.events = EPOLLIN | EPOLLRDHUP;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event);
    setnonblocking(listenfd);

    Sig::init(epollfd);
    addsig(SIGPIPE, SIG_IGN);
    Sig::AddSignal(SIGALRM);
    Sig::AddSignal(SIGTERM);
    Sig::AddSignal(SIGINT);
    alarm(TIMESLOT);

    time_heap = make_shared<TimeHeap>(MAX_FD);
    http_conn::time_heap = time_heap;
}

void WebServer::initHttpConn(int connfd, struct sockaddr_in client_address)
{
    users[connfd]->init(connfd, client_address, root_dir);

    if (opt_linger)
    {
        struct linger linger_ = {1, 1};
        setsockopt(connfd, SOL_SOCKET, SO_LINGER, &linger_, sizeof(linger_));
    }
    //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到时间堆中
    shared_ptr<heap_timer> timer = make_shared<heap_timer>(3 * TIMESLOT);
    timer->user_data = users[connfd];
    timer->cb_func = close_http_conn_cb_func;

    users[connfd]->timer = timer;
    time_heap->add_timer(timer);
}

//若有数据传输，则将定时器往后延迟3个单位
void WebServer::adjustTimer(shared_ptr<heap_timer> timer)
{
    time_heap->adjustTimer(timer, 3 * TIMESLOT);
}

bool WebServer::acceptClient()
{
    struct sockaddr_in client_address;
    // while(1){
    socklen_t client_addrlength = sizeof(client_address);
    int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
    if (connfd < 0)
    {
        LOG_ERROR("%s:errno is:%d\n", "accept error", errno);
        return false;
    }
    if (http_conn::m_user_count >= MAX_FD)
    {
        char info[] = "Internal server busy";
        send(connfd, info, strlen(info), 0);
        close(connfd);
        LOG_ERROR("%s\n", info);
        return false;
    }
    char buf[20];
    LOG_DEBUG("accept client from %s:%d\n", inet_ntop(AF_INET, &client_address.sin_addr, buf, INET_ADDRSTRLEN), ntohs(client_address.sin_port));
    initHttpConn(connfd, client_address);
    // }

    return true;
}

bool WebServer::dealSignal()
{
    int ret = 0;
    char signals[1024];
    ret = recv(Sig::pipefd[0], signals, sizeof(signals), 0);
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
            case SIGINT:
            {
                stop_server = true;
                break;
            }
            default:
            {
                LOG_INFO("get other signal ,deal with default\n");
            }
            }
        }
    }
    return true;
}

void WebServer::dealRead(int sockfd)
{
    shared_ptr<heap_timer> timer = users[sockfd]->timer.lock();
    if (timer)
    {
        adjustTimer(timer);
    }
    else
    {
        timer = make_shared<heap_timer>(3 * TIMESLOT);
        timer->user_data = users[sockfd];
        timer->cb_func = close_http_conn_cb_func;
        users[sockfd]->timer = timer;
        time_heap->add_timer(timer);
    }
    //若监测到读事件，将该事件放入请求队列
    users[sockfd]->setRead();
    thread_pool->append(users[sockfd]); //第二个参数为
}

void WebServer::dealWrite(int sockfd)
{
    shared_ptr<heap_timer> timer = users[sockfd]->timer.lock();
    if (timer)
    {
        adjustTimer(timer);
    }
    else
    {
        users[sockfd]->close_conn();
        return;
    }
    //若监测到写事件，将该事件放入请求队列
    users[sockfd]->setWrite();
    thread_pool->append(users[sockfd]);
}

void WebServer::loop()
{
    timeout = false;
    stop_server = false;

    while (!stop_server)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR)
        {
            LOG_ERROR("epoll failure\n");
            break;
        }
        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            //处理新到的客户连接
            if (sockfd == listenfd)
            {
                acceptClient();
                // reset_oneshot(epollfd, listenfd);
                // while(acceptClient());
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                //如有异常 关闭客户端链接
                users[sockfd]->close_conn();
            }
            //处理信号
            else if ((sockfd == Sig::pipefd[0]) && (events[i].events & EPOLLIN))
            {
                bool flag = dealSignal();
                if (false == flag)
                    LOG_ERROR("deal signal failure\n");
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
            int size = time_heap->size();
            time_heap->tick();
            if(size!=time_heap->size())
                LOG_INFO("timer tick time_heap.size = %d(before):%d\n", size, time_heap->size());
            // printf("keep-alive nums:%d\n",time_heap->size());
            // printf("\033[1A\n");
            LOG_FLUSH();
            time_t tmp = TIMESLOT;
            if (!time_heap->empty() && time_heap->top()->expire < tmp)
            {
                tmp = time_heap->top()->expire;
            }
            alarm(TIMESLOT);
            timeout = false;
        }
    }
}