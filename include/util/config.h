#ifndef WEBSERVER_CONFIG_H_
#define WEBSERVER_CONFIG_H_

#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <cassert>
#include <string>
#include "util.h"
#include "logger.h"
using namespace std;

class Config
{
public:
    Config();
    ~Config(){};

    void parse_arg(int argc, char *argv[]);
    void parse_ini_file(string filename);

    //端口号
    int port;

    //优雅关闭链接
    int linger;

    //线程池内的线程数量
    int thread_num;

    //请求队列中最大请求数量
    int max_request;

    //数据库连接池数量
    int mysql_conn_num;
    string mysql_host;
    string mysql_user;
    string mysql_passwd;
    string mysql_db_name;
    int mysql_port;


    //是否关闭日志
    int close_log;
    LOGLEVEL log_level;
    string log_pre_filename;
    size_t log_buf_size;
    size_t log_max_lines;

private:
    bool parse_line(string line,string &key,string &val);
    void trim(string &str);
};

#endif  //WEBSERVER_CONFIG_H_