#include "config.h"
#include "webserver.h"
#include <string>
using namespace std;
int main(int argc, char *argv[])
{
    //需要修改的数据库信息,登录名,密码,库名
    string user = "root";
    string passwd = "matrix";
    string db_name = "mydb";

    //命令行解析
    Config config;
    config.sql_user = user;
    config.sql_passwd = passwd;
    config.sql_db_name = db_name;
    config.parse_arg(argc, argv);

    WebServer server;

    //初始化
    server.init(config);
    printf("using port:%d",config.port);

    //运行
    server.run();

    return 0;
}