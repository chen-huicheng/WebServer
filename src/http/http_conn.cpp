#include "http_conn.h"

//定义http响应的一些状态信息
const char *ok_200_title = "OK";
const char *redirect_302_title = "Found";
const char *error_400_title = "Bad Request";
const char *redirect_302_from = "redirect";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";
const char *test_reponse = "<html><body>test</body></html>";
const int TIMEOUT = 30; //单位分钟
const int MAXCACHE = 10000;//最大缓存数
int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;
shared_ptr<TimeHeap> http_conn::time_heap;

unordered_map<string,shared_ptr<FileStat> > http_conn::file_cache;
locker http_conn::file_mutex;
Session http_conn::session(MAXCACHE,TIMEOUT);
//关闭连接，关闭一个连接，客户总量减一
void http_conn::close_conn()
{
    //删除与该连接有关的定时器
    time_heap->del_timer(timer.lock());
    timer.reset();
    //从epoll监听中删除 并关闭连接
    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_sockfd, 0);
    close(m_sockfd);
    file_stat.reset();
    m_user_count--;
}

//初始化连接,外部调用初始化套接字地址
void http_conn::init(int sockfd, const sockaddr_in &addr, string root)
{
    m_sockfd = sockfd;
    m_address = addr;
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    addfd(m_epollfd, sockfd, true);
    m_user_count++;

    init();
    doc_root = root;
}

//初始化新接受的连接
//check_state默认为分析请求行状态
void http_conn::init()
{
    bytes_to_send = 0;
    bytes_have_send = 0;
    m_check_state = CHECK_STATE_REQUESTLINE;
    keep_alive = false;
    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    m_state = 0;

    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
    sessionid.clear();
    login_stat = UNLOGIN;
}

//循环读取客户数据，直到无数据可读或对方关闭连接
bool http_conn::read()
{
    if (m_read_idx >= READ_BUFFER_SIZE)
    {
        return false;
    }
    int bytes_read = 0;
    while (true)
    {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if (bytes_read == -1)
        {
            if (errno == EINTR) //读取过程中被中断
            {
                bytes_read = 0;
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK) //非阻塞IO文件描述符 发生阻塞
            {
                break;
            }
            return false;
        }
        else if (bytes_read == 0) //连接被关闭
        {
            return false;
        }
        m_read_idx += bytes_read;
    }
    return true;
}

//从状态机，用于分析出一行内容
//返回值为行的读取状态，有LINE_OK,LINE_BAD,LINE_OPEN
http_conn::LINE_STATUS http_conn::parse_line()
{
    char temp;
    for (; m_checked_idx < m_read_idx; ++m_checked_idx)
    {
        temp = m_read_buf[m_checked_idx];
        if (temp == '\r')
        {
            if ((m_checked_idx + 1) == m_read_idx)
                return LINE_OPEN;
            else if (m_read_buf[m_checked_idx + 1] == '\n')
            {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n')
        {
            if (m_checked_idx > 1 && m_read_buf[m_checked_idx - 1] == '\r')
            {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

//解析http请求行，获得请求方法，目标url及http版本号
http_conn::HTTP_CODE http_conn::parse_request_line(char *text) 
{
    //char *strpbrk(const char *str1, const char *str2) 检索字符串 str1 中第一个匹配字符串 str2 中字符的字符，不包含空结束字符'\0'。
    m_url = strpbrk(text, " \t");
    if (!m_url)
    {
        return BAD_REQUEST;
    }
    *m_url++ = '\0';
    char *method = text;
    if (strcasecmp(method, "GET") == 0) //用忽略大小写比较字符串
        m_method = GET;
    else if (strcasecmp(method, "POST") == 0)
    {
        m_method = POST;
    }
    else
        return BAD_REQUEST;
    m_url += strspn(m_url, " \t"); //size_t strspn(const char *str1, const char *str2) 检索字符串 str1 中第一个不在字符串 str2 中出现的字符下标。
    m_version = strpbrk(m_url, " \t");
    if (!m_version)
        return BAD_REQUEST;
    *m_version++ = '\0';
    m_version += strspn(m_version, " \t");
    if (strcasecmp(m_version, "HTTP/1.1") != 0)
        return BAD_REQUEST;
    if (strncasecmp(m_url, "http://", 7) == 0)
    {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }

    if (strncasecmp(m_url, "https://", 8) == 0)
    {
        m_url += 8;
        m_url = strchr(m_url, '/');
    }

    if (!m_url || m_url[0] != '/')
        return BAD_REQUEST;
    //当url为/时，显示index界面
    if (strlen(m_url) == 1)
        strcat(m_url, "index.html");
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//解析http请求的一个头部信息
http_conn::HTTP_CODE http_conn::parse_headers(char *text)
{
    if (text[0] == '\0') //空行结束头解析
    {
        if (m_content_length != 0)
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0)
    {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0)
        {
            keep_alive = true;
        }
    }
    else if (strncasecmp(text, "Content-length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    }
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    }
    else if(strncasecmp(text,"Cookie:",7)==0)
    {
        text+=7;
        text += strspn(text, " \t");
        char *p = strpbrk(text, ";");
        if(p)
            *p='\0';
        sessionid = string(text);
        login_stat = session.get_status(sessionid);
    }
    else
    {
        LOG_DEBUG("oop!unknow header: %s\n", text);
    }
    return NO_REQUEST;
}

//判断http请求是否被完整读入
http_conn::HTTP_CODE http_conn::parse_content(char *text)
{
    if (m_read_idx >= (m_content_length + m_checked_idx))
    {
        text[m_content_length] = '\0';
        if (m_method == GET)
            return GET_REQUEST;
        else if (m_method == POST)
        {
            //POST请求中最后为输入的用户名和密码
            m_content = text;
            return POST_REQUEST;
        }
    }
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::process_read()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;

    while ((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = parse_line()) == LINE_OK))
    {
        text = get_line();
        m_start_line = m_checked_idx;
        switch (m_check_state)
        {
        case CHECK_STATE_REQUESTLINE:
        {
            LOG_INFO("%s\n", text);
            ret = parse_request_line(text);
            if (ret == BAD_REQUEST)
                return BAD_REQUEST;
            break;
        }
        case CHECK_STATE_HEADER:
        {
            ret = parse_headers(text);
            if (ret == GET_REQUEST)
            {
                return do_get_request();
            }
            break;
        }
        case CHECK_STATE_CONTENT:
        {
            ret = parse_content(text);
            if (ret == GET_REQUEST)
                return do_get_request();
            if (ret == POST_REQUEST)
                return do_post_request();
            line_status = LINE_OPEN;
            break;
        }
        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::do_post_request()
{

    strcpy(m_real_file, doc_root.c_str());
    const char *p = strrchr(m_url, '/'); //char *strrchr(const char *str, int c) 在参数 str 所指向的字符串中搜索最后一次出现字符 c（一个无符号字符）的位置
    p++;
    map<string, string> kv_pair;
    kv_pair = parse_form(m_content);
    if (strlen(p) == 5 && strncmp(p, "login", 5) == 0)
    {
        string username = kv_pair["username"];
        string passwd = kv_pair["passwd"];

        if (login_user(username, passwd))
        {
            m_url = strcat(m_real_file+doc_root.size()+10,"/pages/user/welcome.html");
            sessionid = session.put(username);
            login_stat = FIRST_LOGIN;
        }
        else
        {
            m_url = strcat(m_real_file+doc_root.size()+10,"/pages/loginError.html");
        }
    }
    else if (strlen(p) == 8 && strncmp(p, "register", 8) == 0)
    {
        string username = kv_pair["username"];
        string passwd = kv_pair["passwd"];
        if (register_user(username, passwd))
        {
            m_url = strcat(m_real_file+doc_root.size()+10,"/pages/user/welcome.html");
            sessionid = session.put(username);
            login_stat = FIRST_LOGIN;
        }
        else
        {
            m_url = strcat(m_real_file+doc_root.size()+10,"/pages/registerError.html");
        }
    }
    return do_get_request();
}

http_conn::HTTP_CODE http_conn::do_get_request()
{
    if(strcmp(m_url,"/test")==0)
    {
        return TEST_REQUEST;
    }
    if(strstr(m_url,"/pages/user/")!=NULL){
        if(sessionid.empty()||session.get_status(sessionid)==UNLOGIN)
        {
            return FORBIDDEN_REQUEST;
        }
    }
    strcpy(m_real_file, doc_root.c_str());
    int len = doc_root.size();
    strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
    
    file_mutex.lock();
    if(file_cache.count(m_real_file)){
        file_stat = file_cache[m_real_file];
        file_mutex.unlock();
        return FILE_REQUEST;
    }
    file_mutex.unlock();
    char *address;
    struct stat m_file_stat;
    if (stat(m_real_file, &m_file_stat) < 0)
        return NO_RESOURCE;

    if (!(m_file_stat.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;

    if (S_ISDIR(m_file_stat.st_mode))
        return BAD_REQUEST;

    int fd = open(m_real_file, O_RDONLY);
    address = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    file_stat = make_shared<FileStat>(m_file_stat,address);
    file_mutex.lock();
    file_cache[m_real_file]=file_stat;
    file_mutex.unlock();
    return FILE_REQUEST;
}

void http_conn::unmap()
{
    if(file_cache.size()<CACHE_SIZE){
        return;
    }
    else if (file_stat)
    {
        string file_name;
        uint32_t min_n=UINT32_MAX;
        file_mutex.lock();
        if(file_cache.size()<CACHE_SIZE)
        {
            file_mutex.unlock();
            return;
        }
        for(auto it:file_cache){
            if(it.second->usage_times<min_n){
                min_n = it.second->usage_times;
                file_name=it.first;
            }
        }
        file_cache.erase(file_name);
        file_mutex.unlock();
    }
}

bool http_conn::write()
{
    int temp = 0;
    if (bytes_to_send == 0)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        init();
        return true;
    }

    while (1)
    {
        temp = writev(m_sockfd, m_iv, m_iv_count);

        if (temp < 0)
        {
            if (errno == EINTR) //读取过程中被中断
            {
                temp = 0;
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }
        bytes_have_send += temp;
        bytes_to_send -= temp;
        if (bytes_to_send <= 0)
        {
            unmap();
            if (keep_alive)
            {
                modfd(m_epollfd, m_sockfd, EPOLLIN);
                init();
                return true;
            }
            else
            {
                return false;
            }
        }
        if (m_iv_count>1&&bytes_have_send >= m_iv[0].iov_len)
        {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = file_stat->address + (bytes_have_send - m_write_idx);
            m_iv[1].iov_len = bytes_to_send;
        }
        else
        {
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
        }

    }
}

bool http_conn::add_response(const char *format, ...)
{
    if (m_write_idx >= WRITE_BUFFER_SIZE)
        return false;
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
    {
        va_end(arg_list);
        return false;
    }
    // LOG_INFO("response: %s", m_write_buf+m_write_idx);
    m_write_idx += len;
    va_end(arg_list);
    return true;
}

bool http_conn::add_status_line(int status, const char *title)
{
    LOG_INFO("response: %s %d %s\r\n", "HTTP/1.1", status, title);
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http_conn::add_headers(int content_len)
{
    return add_content_length(content_len) && add_linger() &&
           add_blank_line();
}

bool http_conn::add_content_length(int content_len)
{
    LOG_INFO("Content-Length:%d\r\n", content_len);
    return add_response("Content-Length:%d\r\n", content_len);
}

bool http_conn::add_content_type()
{
    LOG_INFO("Content-Type:%s\r\n", "text/html");
    return add_response("Content-Type:%s\r\n", "text/html");
}
bool http_conn::add_cookie()
{
    LOG_INFO("set-cookie:%s\r\n", "text/html");
    if(!sessionid.empty()&&login_stat==FIRST_LOGIN)
        return add_response("set-cookie:%s\r\n", sessionid.c_str());
    return true;
}
bool http_conn::add_linger()
{
    LOG_INFO("Connection:%s\r\n", (keep_alive == true) ? "keep-alive" : "close");
    return add_response("Connection:%s\r\n", (keep_alive == true) ? "keep-alive" : "close");
}

bool http_conn::add_blank_line()
{
    return add_response("%s", "\r\n");
}

bool http_conn::add_content(const char *content)
{
    LOG_INFO("%s\n", content);
    return add_response(content);
}
bool http_conn::add_test_reponse()
{
    LOG_INFO("%s\n", test_reponse);
    add_headers(strlen(test_reponse));
    return add_content(test_reponse);
}

bool http_conn::process_write(HTTP_CODE ret)
{
    switch (ret)
    {
    case INTERNAL_ERROR:
    {
        add_status_line(500, error_500_title);
        add_headers(strlen(error_500_form));
        if (!add_content(error_500_form))
            return false;
        break;
    }
    case BAD_REQUEST:
    {
        add_status_line(404, error_404_title);
        add_headers(strlen(error_404_form));
        if (!add_content(error_404_form))
            return false;
        break;
    }
    case FORBIDDEN_REQUEST:
    {
        add_status_line(302, redirect_302_title);
        add_response("Location:/index.html\r\n");
        add_blank_line();       //必须添加空行 不然客户端无法判断数据是否发送完
        // add_status_line(403, error_403_title);
        // add_headers(strlen(error_403_form));
        // if (!add_content(error_403_form))
            // return false;
        break;
    }
    case FILE_REQUEST:
    {
        add_status_line(200, ok_200_title);
        add_cookie();
        if (file_stat->status.st_size != 0)
        {
            add_headers(file_stat->status.st_size);
            m_iv[0].iov_base = m_write_buf;
            m_iv[0].iov_len = m_write_idx;
            m_iv[1].iov_base = file_stat->address;
            m_iv[1].iov_len = file_stat->status.st_size;
            m_iv_count = 2;
            bytes_to_send = m_write_idx + file_stat->status.st_size;
            return true;
        }
        else
        {
            const char *ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string));
            if (!add_content(ok_string))
                return false;
        }
        break;
    }
    case TEST_REQUEST:
    {
        add_status_line(200, ok_200_title);
        add_test_reponse();
        break;
    }
    default:
        return false;
    }
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    return true;
}

void http_conn::process()
{
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }
    bool write_ret = process_write(read_ret);
    if (!write_ret)
    {
        close_conn();
    }
    else if (!write())
    {
        close_conn();
    }
    // modfd(m_epollfd, m_sockfd, EPOLLOUT);
}

void http_conn::run()
{
    if (0 == m_state)
    {
        if (read())
        {
            process();
        }
        else
        {
            close_conn();
        }
    }
    else
    {
        if (!write())
        {
            close_conn();
        }
    }
}
