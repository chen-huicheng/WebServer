#ifndef WEBSERVER_LOG_H_
#define WEBSERVER_LOG_H_
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
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

class Log
{
public:
    //C++11使用局部变量懒汉不用加锁
    static Log *get_instance()
    {
        static Log instance;
        return &instance;
    }
    bool init(string filename, bool close_log, size_t log_buf_size = 8192, size_t log_max_lines = 5000000, LOGLEVEL level = INFO);

    void write_log(LOGLEVEL level, const char *msg, ...);

    void flush(void);

    void set_level(LOGLEVEL level)
    {
        cur_level_ = level;
    }
    LOGLEVEL get_level() const
    {
        return cur_level_;
    }

private:
    Log(){};
    ~Log(){};
    string levelstr(LOGLEVEL level);
    

private:
    bool close_log_;        //是否关闭日志
    LOGLEVEL cur_level_;    //默认 INFO
    LogStream logstream_;   //缓冲区 延迟写入到文件
    const int log_buf_size_=1024; //日志buf大小
};

#define LOG_DEBUG(msg, args...)                             \
    {                                                       \
        Log::get_instance()->write_log(DEBUG, msg, ##args); \
    }
#define LOG_INFO(msg, args...)                             \
    {                                                      \
        Log::get_instance()->write_log(INFO, msg, ##args); \
    }
#define LOG_WARN(msg, args...)                             \
    {                                                      \
        Log::get_instance()->write_log(WARN, msg, ##args); \
    }
#define LOG_ERROR(msg, args...)                             \
    {                                                       \
        Log::get_instance()->write_log(ERROR, msg, ##args); \
    }
#define LOG_FLUSH() Log::get_instance()->flush()
#endif //WEBSERVER_LOG_H_