#include "webserver.h"
#include "util.h"
WebServer::WebServer()
{
    //http_conn类对象
    users = new http_conn[MAX_FD];

    //root文件夹路径
    char server_path[200];
    getcwd(server_path, 200);
    char root[6] = "/root";
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root, server_path);
    strcat(m_root, root);

    //定时器
    users_timer = new client_data[MAX_FD];
}

WebServer::~WebServer()
{
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    delete[] users;
    delete[] users_timer;
    delete m_pool;
}

void WebServer::init(Config config)
{
    m_port = config.port;
    m_user = config.sql_user;
    m_passwd = config.sql_passwd;
    m_db_name = config.sql_db_name;
    m_sql_num = config.sql_num;
    m_thread_num = config.thread_num;
    m_opt_linger = config.opt_linger;
    m_close_log = config.close_log;
}

void WebServer::log_write()
{
    if (0 == m_close_log)
    {
        Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000);
    }
}

void WebServer::sql_pool()
{
    //初始化数据库连接池
    ConnectionPool::GetInstance()->init("localhost", m_user, m_passwd, m_db_name, 3306, m_sql_num);

    //初始化数据库读取表
    users->initmysql_result();
}

void WebServer::thread_pool()
{
    //线程池
    m_pool = new threadpool<http_conn>(m_thread_num,10000); //TODO:max_request
}

void WebServer::eventListen()
{
    m_listenfd = open_listenfd(m_port);
    utils.init(TIMESLOT);

    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    addfd(m_epollfd, m_listenfd, false);//TODO:ET mode
    http_conn::m_epollfd = m_epollfd;

    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);
    setnonblocking(m_pipefd[1]);
    addfd(m_epollfd, m_pipefd[0], false);

    utils.addsig(SIGPIPE, SIG_IGN);
    utils.addsig(SIGALRM, utils.sig_handler, false);
    utils.addsig(SIGTERM, utils.sig_handler, false);

    alarm(TIMESLOT);

    //工具类,信号和描述符基础操作
    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;
}

void WebServer::timer(int connfd, struct sockaddr_in client_address)
{
    users[connfd].init(connfd, client_address,m_root);

    //初始化client_data数据
    //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;
    util_timer *timer = new util_timer;
    timer->user_data = &users_timer[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    users_timer[connfd].timer = timer;
    utils.m_timer_lst.add_timer(timer);
}

//若有数据传输，则将定时器往后延迟3个单位
//并对新的定时器在链表上的位置进行调整
void WebServer::adjust_timer(util_timer *timer)
{
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    utils.m_timer_lst.adjust_timer(timer);

    LOG_INFO("%s", "adjust timer once");
}

void WebServer::deal_timer(util_timer *timer, int sockfd)
{
    timer->cb_func(&users_timer[sockfd]);
    if (timer)
    {
        utils.m_timer_lst.del_timer(timer);
    }

    LOG_INFO("close fd %d", users_timer[sockfd].sockfd);
}

bool WebServer::dealclinetdata()
{
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
    if (connfd < 0)
    {
        LOG_ERROR("%s:errno is:%d", "accept error", errno);
        return false;
    }
    if (http_conn::m_user_count >= MAX_FD)
    {
        utils.show_error(connfd, "Internal server busy");
        LOG_ERROR("%s", "Internal server busy");
        return false;
    }
    timer(connfd, client_address);
    return true;
}

bool WebServer::dealwithsignal(bool &timeout, bool &stop_server)
{
    int ret = 0;
    char signals[1024];
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
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
            }
        }
    }
    return true;
}

void WebServer::dealwithread(int sockfd)
{
    util_timer *timer = users_timer[sockfd].timer;

    if (timer)
    {
        adjust_timer(timer);
    }

    //若监测到读事件，将该事件放入请求队列
    m_pool->append(users + sockfd,0);

    // while (true)
    // {
    //     if (1 == users[sockfd].improv)
    //     {
    //         if (1 == users[sockfd].timer_flag)
    //         {
    //             deal_timer(timer, sockfd);
    //             users[sockfd].timer_flag = 0;
    //         }
    //         users[sockfd].improv = 0;
    //         break;
    //     }
    // }
}

void WebServer::dealwithwrite(int sockfd)
{
    util_timer *timer = users_timer[sockfd].timer;
    //reactor
    if (timer)
    {
        adjust_timer(timer);
    }
    m_pool->append(users + sockfd,1);

    while (true)
    {
        if (1 == users[sockfd].improv)
        {
            if (1 == users[sockfd].timer_flag)
            {
                deal_timer(timer, sockfd);
                users[sockfd].timer_flag = 0;
            }
            users[sockfd].improv = 0;
            break;
        }
    }
}

void WebServer::eventLoop()
{
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server)
    {
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR)
        {
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;

            //处理新到的客户连接
            if (sockfd == m_listenfd)
            {
                bool flag = dealclinetdata();
                if (false == flag)
                    continue;
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                //服务器端关闭连接，移除对应的定时器
                util_timer *timer = users_timer[sockfd].timer;
                deal_timer(timer, sockfd);
            }
            //处理信号
            else if ((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN))
            {
                bool flag = dealwithsignal(timeout, stop_server);
                if (false == flag)
                    LOG_ERROR("%s", "dealclientdata failure");
            }
            //处理客户连接上接收到的数据
            else if (events[i].events & EPOLLIN)
            {
                dealwithread(sockfd);
            }
            else if (events[i].events & EPOLLOUT)
            {
                dealwithwrite(sockfd);
            }
        }
        if (timeout)
        {
            utils.timer_handler();

            LOG_INFO("%s", "timer tick");

            timeout = false;
        }
    }
}