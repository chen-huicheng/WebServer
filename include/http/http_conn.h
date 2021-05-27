#ifndef WEBSERVER_HTTP_CONN_H_
#define WEBSERVER_HTTP_CONN_H_

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <string>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "locker.h"
#include "connection_pool.h"
#include "log.h"
#include "timer.h"
#include "util.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 2048;
    enum METHOD    //http 头部方法
    {
        GET = 0,
        POST
    };
    enum CHECK_STATE //http请求解析状态
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE  //响应码
    {
        NO_REQUEST,
        GET_REQUEST,
        POST_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void run();
    void init(int sockfd, const sockaddr_in &addr, string root);
    int getSockfd()
    {
        return m_sockfd;
    }
    void setWrite()
    {
        m_state = 1;
    }
    void setRead()
    {
        m_state = 0;
    }
    void close_conn();

private:
    void init();  //初始化
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_get_request();
    HTTP_CODE do_post_request();
    char *get_line() { return m_read_buf + m_start_line; };
    LINE_STATUS parse_line();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

    void process();
    bool read();
    bool write();

public:
    static int m_epollfd;
    static int m_user_count;
    static shared_ptr<TimeHeap> time_heap;
    weak_ptr<heap_timer> timer;

private:
    int m_state; //读为0, 写为1

private:
    int m_sockfd;
    sockaddr_in m_address;
    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx;
    int m_checked_idx;
    int m_start_line;
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;
    CHECK_STATE m_check_state;
    METHOD m_method;
    char m_real_file[FILENAME_LEN];
    char *m_url;
    char *m_version;
    char *m_host;
    int m_content_length;
    bool keep_alive;
    char *m_file_address;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;

    char *m_content; //存储请求头数据
    size_t bytes_to_send;
    size_t bytes_have_send;
    string doc_root;
};
#endif //WEBSERVER_HTTP_CONN_H_
