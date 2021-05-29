#ifndef WEBSERVER_LOG_H_
#define WEBSERVER_LOG_H_
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <time.h>
#include <sys/time.h>
#include "locker.h"
#include "logstream.h"

using namespace std;

enum LOGLEVEL
{ //日志级别
    DEBUG = 0,
    INFO,
    WARN,
    ERROR
};

class Logger
{
public:
    //C++11使用局部变量懒汉不用加锁
    static Logger *get_instance()
    {
        static Logger instance;
        return &instance;
    }
    //初始化日志　filename:日志文件名　close_log:是否关闭日志　log_buf_size:日志缓冲区大小　log_max_lines:日志文件存储最对行数　level:日志级别
    bool init(string filename, bool close_log, size_t log_buf_size = 4096*8, size_t log_max_lines = 5000000, LOGLEVEL level = INFO);
    //写日志借口
    void write_log(LOGLEVEL level, const char *msg, ...);
    //刷新日志到磁盘
    void flush(void);
    //设置日志写级别　　仅写入大于日志级别的到磁盘　
    void set_level(LOGLEVEL level)
    {
        cur_level_ = level;
    }
    //获取当前日志级别
    LOGLEVEL get_level() const
    {
        return cur_level_;
    }

private:
    Logger(){};
    ~Logger(){};
    //日志级别转换为string
    string levelstr(LOGLEVEL level);
    

private:
    bool close_log_;        //是否关闭日志
    LOGLEVEL cur_level_;    //默认 INFO
    LogStream logstream_;   //缓冲区 延迟写入到文件
    const int log_buf_size_=1024; //日志buf大小
};

#define LOG_DEBUG(msg, args...)                             \
    {                                                       \
        Logger::get_instance()->write_log(DEBUG, msg, ##args); \
    }
#define LOG_INFO(msg, args...)                             \
    {                                                      \
        Logger::get_instance()->write_log(INFO, msg, ##args); \
    }
#define LOG_WARN(msg, args...)                             \
    {                                                      \
        Logger::get_instance()->write_log(WARN, msg, ##args); \
    }
#define LOG_ERROR(msg, args...)                             \
    {                                                       \
        Logger::get_instance()->write_log(ERROR, msg, ##args); \
    }
#define LOG_FLUSH() Logger::get_instance()->flush()
#endif //WEBSERVER_LOG_H_