#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"
using namespace std;

Log::Log()
{
    log_lines_ = 0;
}

Log::~Log()
{
    if (NULL != fp)
    {
        fclose(fp);
    }
}

const char *Log::levelstr(LOGLEVEL level)
{
    switch (level)
    {
    case DEBUG:
        return "[DBG]:";
    case INFO:
        return "[INF]:";
    case WARN:
        return "[WAR]:";
    case ERROR:
        return "[ERR]:";
    default:
        return "[???]:";
    }
}

bool Log::init(const char *file_name, int close_log, int log_buf_size, int log_max_lines,LOGLEVEL level)
{
    close_log_ = close_log;
    log_buf_size_ = log_buf_size;
    buf = new char[log_buf_size];
    next_buf = new char[log_buf_size];
    memset(buf, '\0', log_buf_size);
    memset(next_buf, '\0', log_buf_size);
    log_max_lines_ = log_max_lines;
    cur_level_ = level;

    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    const char *p = strrchr(file_name, '/');
    char log_full_name[256] = {0};

    if (NULL == p)
    {
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s.log", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    }
    else
    {
        strcpy(log_name_, p + 1);
        strncpy(dir_name_, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s.log", dir_name_, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name_);
    }

    today = my_tm.tm_mday;

    fp = fopen(log_full_name, "a");
    if (NULL == fp)
    {
        return false;
    }
    return true;
}

void Log::write_log(LOGLEVEL level, const char *msg, ...)
{
    if (level < cur_level_ || close_log_)
        return;
    if (close_log_)
        return;
    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    const char *str = levelstr(level);

    //写入一个log
    mutex.lock();
    log_lines_++;

    if (today != my_tm.tm_mday || log_lines_ % log_max_lines_ == 0) //everyday log
    {
        char new_log[256];
        fflush(fp);
        fclose(fp);
        char tail[16];

        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (today != my_tm.tm_mday)
        {
            snprintf(new_log, 255, "%s%s%s.log", dir_name_, tail, log_name_);
            today = my_tm.tm_mday;
            log_lines_ = 0;
        }
        else
        {
            snprintf(new_log, 255, "%s%s%s_%lld.log", dir_name_, tail, log_name_, log_lines_ / log_max_lines_);
        }
        fp = fopen(new_log, "a");
    }

    mutex.unlock();

    va_list valst;
    va_start(valst, msg);

    mutex.lock();


    //写入的具体时间内容格式
    if(buf_cur_idx+48>=log_buf_size_){
        fputs(buf, fp);
        fflush(fp);
        memset(buf, '\0', log_buf_size_);
        buf_cur_idx=0;
    }
    int n = snprintf(buf+buf_cur_idx, 48, "%d-%02d-%02d %02d:%02d:%02d %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, str);

    buf_cur_idx+=n;

    int m = vsnprintf(buf + buf_cur_idx, log_buf_size_ - 1, msg, valst);
    if(m+buf_cur_idx>=log_buf_size_||m<0){
        buf[buf_cur_idx]='\0';
        fputs(buf, fp);
        fflush(fp);
        memset(buf, '\0', log_buf_size_);
        buf_cur_idx = vsnprintf(buf, log_buf_size_ - 1, msg, valst);
        if(buf_cur_idx<0||buf_cur_idx>=log_buf_size_)
        {
            memset(buf, '\0', log_buf_size_);
            buf_cur_idx=0;
            mutex.unlock();
            return;
        }
    }else
        buf_cur_idx+=m;
    // buf[buf_cur_idx]='\0';
    // fputs(buf, fp);
    // fflush(fp);
    // memset(buf, '\0', log_buf_size_);
    // buf_cur_idx=0;


    mutex.unlock();
    
    va_end(valst);
}

void Log::flush(void)
{
    mutex.lock();
    //强制刷新写入流缓冲区
    fputs(buf, fp);
    fflush(fp);
    memset(buf, '\0', log_buf_size_);
    buf_cur_idx=0;
    mutex.unlock();
}
