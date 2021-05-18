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
    config.sql_user = "root";
    config.sql_passwd="matrix";
    config.sql_db_name = "mydb";
    config.parse_arg(argc, argv);

    WebServer server;

    //初始化
    server.init(config);
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