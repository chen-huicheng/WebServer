#include "config.h"
#include "webserver.h"
#include "logger.h"
#include "connection_pool.h"
#include <string>
using namespace std;
int main(int argc, char *argv[])
{
    //命令行解析
    Config config;
    config.parse_ini_file("config.ini");
    config.parse_arg(argc, argv);

    //日志初始化
    Logger::get_instance()->init(config.log_pre_filename, config.close_log, config.log_buf_size, config.log_max_lines);
    Logger::get_instance()->set_level(config.log_level);

    //数据库连接池初始化
    ConnectionPool::GetInstance()->init(config.mysql_host, config.mysql_user, config.mysql_passwd, config.mysql_db_name, config.mysql_port, config.mysql_conn_num);

    //webserver初始化
    WebServer server;
    server.init(config);
    LOG_INFO("using port:%d\n", config.port);
    //运行
    server.loop();

    return 0;
}