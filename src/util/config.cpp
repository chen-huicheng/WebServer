#include "config.h"
#include "util.h"
Config::Config()
{
    //端口号,默认1234
    port = 1234;

    //优雅关闭链接，默认不使用
    opt_linger = 0;

    //数据库连接池数量,默认8
    sql_num = 8;

    //线程池内的线程数量,默认5
    thread_num = 5;

    //关闭日志,默认不关闭
    close_log = 0;

    //请求队列中最大请求数量
    max_request = 10000;
}

void Config::parse_arg(int argc, char *argv[])
{
    int opt;
    const char *str = "p:l:s:t:c:r:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            port = atoi(optarg);
            break;
        }
        case 'l':
        {
            opt_linger = atoi(optarg);
            break;
        }
        case 's':
        {
            sql_num = atoi(optarg);
            break;
        }
        case 't':
        {
            thread_num = atoi(optarg);
            break;
        }
        case 'c':
        {
            close_log = atoi(optarg);
            break;
        }
        case 'r':
        {
            max_request = atoi(optarg);
            break;
        }
        default:
            usage();
            abort();
            break;
        }
    }
}
