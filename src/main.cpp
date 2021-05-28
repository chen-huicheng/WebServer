#include "config.h"
#include "webserver.h"
#include <string>
using namespace std;
int main(int argc, char *argv[])
{
    //命令行解析
    Config config;
    config.parse_arg(argc, argv);

    //需要修改的数据库信息,登录名,密码,库名
     //数据库连接池初始化
    string user = "root";
    string passwd = "matrix";
    string db_name = "mydb";
    ConnectionPool::GetInstance()->init("localhost", user, passwd, db_name, 3306, config.sql_num);


    //日志初始化
    string log_pre_filename="Serverlog";
    
    const int log_buf_size=1024*4*16;
    const int log_max_lines=1000000;
    Logger::get_instance()->init(log_pre_filename, config.close_log, log_buf_size, log_max_lines);


    WebServer server;
    //webserver初始化
    server.init(config);
    printf("using port:%d\n", config.port);
    //运行
    server.loop();

    return 0;
}