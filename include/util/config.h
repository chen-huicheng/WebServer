#ifndef WEBSERVER_CONFIG_H_
#define WEBSERVER_CONFIG_H_

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <string>
using namespace std;

class Config
{
public:
    Config();
    ~Config(){};

    void parse_arg(int argc, char *argv[]);

    //端口号
    int port;

    //优雅关闭链接
    int opt_linger;

    //数据库连接池数量
    int sql_num;

    //线程池内的线程数量
    int thread_num;

    //是否关闭日志
    int close_log;

    string sql_user;
    string sql_passwd;
    string sql_db_name;
};

#endif  //WEBSERVER_CONFIG_H_