#include "config.h"
#include "webserver.h"
#include <string>
using namespace std;
int main(int argc, char *argv[])
{
    //需要修改的数据库信息,登录名,密码,库名
    string user = "root";
    string passwd = "matrix";
    string databasename = "mydb";

    //命令行解析
    Config config;
    config.parse_arg(argc, argv);

    WebServer server;

    //初始化
    server.init(config.PORT, user, passwd, databasename, config.OPT_LINGER, 
                config.sql_num,  config.thread_num, config.close_log);
    //日志
    server.log_write();

    //数据库
    server.sql_pool();

    //线程池
    server.thread_pool();


    //监听
    server.eventListen();

    //运行
    server.eventLoop();

    return 0;
}