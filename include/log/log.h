#ifndef WEBSERVER_LOG_H_
#define WEBSERVER_LOG_H_
#include <stdio.h>
#include <stdarg.h>
#include "locker.h"

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

    //可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int log_max_lines_ = 5000000, LOGLEVEL level = INFO);

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
    Log();
    ~Log();
    const char *levelstr(LOGLEVEL level);

private:
    char dir_name_[128];  //路径名
    char log_name_[128];  //log文件名
    int log_max_lines_;   //日志最大行数
    int log_buf_size_;    //日志缓冲区大小
    long long log_lines_; //日志行数记录
    int today;            //因为按天分类,记录当前时间是那一天
    FILE *fp;             //打开log的文件指针
    char *buf;
    char *next_buf;
    int buf_cur_idx;
    locker mutex;
    bool close_log_;
    LOGLEVEL cur_level_; //默认 INFO
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