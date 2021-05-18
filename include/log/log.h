#pragma once

#include <stdio.h>
#include <stdarg.h>
#include "locker.h"

using namespace std;

class Log
{
public:
    //C++11以后,使用局部变量懒汉不用加锁
    static Log *get_instance()
    {
        static Log instance;
        return &instance;
    }

    //可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int log_max_lines_ = 5000000);

    void write_log(int level, const char *format, ...);

    void flush(void);

private:
    Log();
    ~Log();

private:
    char dir_name_[128];  //路径名
    char log_name_[128];  //log文件名
    int log_max_lines_;   //日志最大行数
    int log_buf_size_;    //日志缓冲区大小
    long long log_lines_; //日志行数记录
    int today;            //因为按天分类,记录当前时间是那一天
    FILE *fp;             //打开log的文件指针
    char *buf;
    locker mutex;
    bool close_log_;
};

#define LOG_DEBUG(format, ...)                                    \
    {                                                             \
        Log::get_instance()->write_log(0, format, ##__VA_ARGS__); \
        Log::get_instance()->flush();                             \
    }
#define LOG_INFO(format, ...)                                     \
    {                                                             \
        Log::get_instance()->write_log(1, format, ##__VA_ARGS__); \
        Log::get_instance()->flush();                             \
    }
#define LOG_WARN(format, ...)                                     \
    {                                                             \
        Log::get_instance()->write_log(2, format, ##__VA_ARGS__); \
        Log::get_instance()->flush();                             \
    }
#define LOG_ERROR(format, ...)                                    \
    {                                                             \
        Log::get_instance()->write_log(3, format, ##__VA_ARGS__); \
        Log::get_instance()->flush();                             \
    }
