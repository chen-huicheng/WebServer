#include "logstream.h"
#include "logger.h"
//初始化日志　参数
LogStream::LogStream()
{
    buf_ = nullptr;
    stop = false;
    cur_lines_ = 0;
    buf_in_ = 0;
    buf_out_ = 0;
    num_ = 0;
};
//日志缓冲　析构函数　刷新日志　结束线程　关闭文件描述符
LogStream::~LogStream()
{
    stop = true;
    cond_.signal();
    flush();
    fclose(fp_);
    delete buf_;
    delete next_buf_;
    delete write_buf_;
};
//初始化日志
bool LogStream::init(string pre_filename, size_t buf_size, size_t max_lines)
{
    pre_filename_ = pre_filename;
    max_lines_ = max_lines;
    buf_size_ = buf_size;
    today_ = getTime();
    full_name_ = pre_filename_ + "_" + today_ + "_" + to_string(num_) + ".log";
    fp_ = fopen(full_name_.c_str(), "ae");
    if(fp_<0){
        LOG_ERROR("log file open failed!!");
        return false;
    }

    buf_size_ += 1023;
    buf_size_ &= ~1023;
    buf_ = new char[buf_size_];      //新建一个日志缓冲池
    next_buf_ = new char[buf_size_]; //备用日志缓冲池
    write_buf_ = nullptr;

    pthread_t tid; //定义日志刷新线程　缓冲区满时刷新
    if (pthread_create(&tid, NULL, writeToFile, this) != 0)
    {
        return false;
    }
    if (pthread_detach(tid))
    {
        return false;
    }
    return true;
}
int LogStream::write(char *line, size_t len)
{
    if (nullptr == line)
        return 0;
    if (nullptr == buf_ || len > buf_size_) //写入日志缓冲为空　写入内容大于缓冲区直接写入到文件
    {
        *(line + len) = '\0';
        fmutex.lock();
        int ret = fputs(line, fp_);
        fmutex.unlock();
        return ret;
    }
    mutex.lock();                  //写入缓冲区前加锁
    if (buf_in_ + len > buf_size_) //当前缓冲区满　
    {
        buf_[buf_in_] = '\0';
        buf_in_ = 0;
        if (nullptr == next_buf_) //备用缓冲区空（原因：write_buf_还未写入到文件）　将当前缓冲区写入到文件
        {
            fmutex.lock();
            fputs(buf_, fp_);
            fflush(fp_);
            fmutex.unlock();
        }
        else //使用备用缓冲区替换　写入缓冲区　并将写满的缓冲区交给输出缓冲区
        {
            write_buf_ = buf_;
            buf_ = next_buf_;
            next_buf_ = nullptr;
            cond_.signal();
        }
    }
    //将日志写入到缓冲区
    strncpy(buf_ + buf_in_, line, len);
    buf_in_ += len;
    cur_lines_++;
    mutex.unlock();
    return len;
}
int LogStream::flush()
{
    //刷新　write_buf_
    mutex.lock();
    flushWriteBuf();
    //将当前缓冲区内容交给　write_buf_
    buf_[buf_in_] = '\0';
    buf_in_ = 0;
    write_buf_ = buf_;
    buf_ = next_buf_;
    next_buf_ = nullptr;
    mutex.unlock();
    return flushWriteBuf();//再次刷新　write_buf_
}

void LogStream::run()
{
    while (!stop)
    {
        cond_.wait();//等待　write_buf_　有被赋值
        flushWriteBuf();
        if (cur_lines_ > max_lines_)    //根据写入行数　更改文件名
        {
            if (getTime() == today_)
            {
                full_name_ = pre_filename_ + "_" + today_ + "_" + to_string(num_) + ".log";
                num_++;
            }
            else
            {
                num_ = 0;
                today_ = getTime();
                full_name_ = pre_filename_ + "_" + today_ + "_" + to_string(num_) + ".log";
            }
            cur_lines_ = 0;
            FILE *fp = fopen(full_name_.c_str(), "ae");
            fmutex.lock();
            FILE *tmp = fp_;
            fp_ = fp;
            fmutex.unlock();
            fclose(tmp);
        }
    }
}
int LogStream::flushWriteBuf() //刷新　write_buf_
{
    int ret=0;
    fmutex.lock();
    if (nullptr != write_buf_)
    {
        fputs(write_buf_, fp_);
        next_buf_ = write_buf_;
        write_buf_ = nullptr;
        ret = fflush(fp_);
    }
    fmutex.unlock();
    return ret;
}
void *LogStream::writeToFile(void *arg)     //线程执行函数　调用LogStream.run　执行真正内容
{
    LogStream *logstream = (LogStream *)arg;
    logstream->run();
    return logstream;
}
string LogStream::getTime()
{
    time_t t;
    struct tm *sys_time;
    char buffer[64];
    time(&t);

    sys_time = localtime(&t);

    strftime(buffer, 60, "%Y_%m_%d", sys_time);
    return buffer;
}