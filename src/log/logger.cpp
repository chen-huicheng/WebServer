#include "logger.h"

string Logger::levelstr(LOGLEVEL level)
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

bool Logger::init(string file_name, bool close_log, size_t log_buf_size, size_t log_max_lines, LOGLEVEL level)
{
    close_log_ = close_log;
    cur_level_ = level;
    if(!logstream_.init(file_name, log_buf_size, log_max_lines))
    {
        LOG_ERROR("logger init failed!!");
        return false;
    }
    return true;
}

void Logger::write_log(LOGLEVEL level, const char *msg, ...)
{
    char buf[1024] = {0}; //日志buf
    int buf_cur_idx = 0;
    if (level < cur_level_ || close_log_)
        return;
    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    string str = levelstr(level);

    //写入一个log
    va_list valst;
    va_start(valst, msg);

    //写入的具体时间内容格式
    int rt = snprintf(buf + buf_cur_idx, 48, "%d-%02d-%02d %02d:%02d:%02d %s ",
                      my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                      my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, str.c_str());

    buf_cur_idx += rt;

    rt = vsnprintf(buf + buf_cur_idx, log_buf_size_ - 1, msg, valst);
    if (rt + buf_cur_idx >= log_buf_size_ || rt < 0)
    {
        printf("log is too long can't write to logbuf!!\n");
        buf[log_buf_size_ - 1] = '\0';
        printf("log:%s\n", buf);
    }
    buf_cur_idx += rt;
    //调用logstream写入具体内容
    logstream_.write(buf, buf_cur_idx);

    va_end(valst);
}

void Logger::flush(void)
{
    //调用logstream的flush刷新缓冲区
    logstream_.flush();
}
