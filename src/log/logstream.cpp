#include "logstream.h"

LogStream::LogStream()
{
    buf_ = nullptr;
    stop = false;
    cur_lines_ = 0;
    buf_in_ = 0;
    buf_out_ = 0;
    num_ = 0;
};
LogStream::~LogStream()
{
    stop = true;
    delete buf_;
    delete next_buf_;
    delete write_buf_;
};
void LogStream::init(string pre_filename, size_t buf_size, size_t max_lines)
{
    pre_filename_ = pre_filename;
    max_lines_ = max_lines;
    buf_size_ = buf_size;
    today_ = getTime();
    full_name_ = pre_filename_ + "_" + today_ + ".log";
    fp_ = fopen(full_name_.c_str(), "ae");

    buf_size_ += 1024;
    buf_size_ &= ~1023;
    buf_ = new char[buf_size_];
    next_buf_ = new char[buf_size_];
    write_buf_ = nullptr;

    pthread_t tid;
    if (pthread_create(&tid, NULL, writeToFile, this) != 0)
    {
        throw std::exception();
    }
    if (pthread_detach(tid))
    {
        throw std::exception();
    }
}
int LogStream::write(char *line, int len)
{
    if (nullptr == buf_)
    {
        printf("not init buf\n");
        assert(nullptr != buf_);
    }
    if (nullptr == line)
        return 0;
    if (len > buf_size_)
    {
        *(line + len) = '\0';
        fmutex.lock();
        int ret = fputs(line, fp_);
        fmutex.unlock();
        return ret;
    }
    mutex.lock();
    if (buf_in_ + len > buf_size_)
    {
        buf_[buf_in_] = '\0';
        buf_in_ = 0;
        if (nullptr == next_buf_)
        {
            fmutex.lock();
            fputs(buf_, fp_);
            fflush(fp_);
            fmutex.unlock();
        }
        else
        {
            write_buf_ = buf_;
            buf_ = next_buf_;
            next_buf_ = nullptr;
            cond_.signal();
        }
    }
    strncpy(buf_ + buf_in_, line, len);
    buf_in_ += len;
    lines_++;
    mutex.unlock();
}
int LogStream::flush()
{
    flushWriteBuf();
    mutex.lock();
    buf_[buf_in_] = '\0';
    buf_in_ = 0;
    write_buf_ = buf_;
    buf_ = next_buf_;
    next_buf_ = nullptr;
    mutex.unlock();
    flushWriteBuf();
}

void LogStream::run()
{
    while (!stop)
    {
        cond_.wait();
        flushWriteBuf();
        if (lines_ > max_lines_)
        {
            if (getTime() == today_)
            {
                full_name_ = pre_filename_ + "_" + getTime() + "_" + to_string(num_) + ".log";
                num_++;
            }
            else
            {
                full_name_ = pre_filename_ + "_" + today_ + ".log";
                num_ = 0;
            }
            FILE *fp = fopen(full_name_.c_str(), "ae");
            fmutex.lock();
            FILE *tmp = fp_;
            fp_ = fp;
            fmutex.unlock();
            fclose(tmp);
        }
    }
}
int LogStream::flushWriteBuf()
{
    fmutex.lock();
    if (nullptr != write_buf_)
    {
        fputs(write_buf_, fp_);
        next_buf_ = write_buf_;
        write_buf_ = nullptr;
        fflush(fp_);
    }
    fmutex.unlock();
}
static void *LogStream::writeToFile(void *arg)
{
    LogStream *logstream = (LogStream *)arg;
    logstream->run();
}
string LogStream::getTime()
{
    time_t t;
    struct tm *sys_time;
    char buffer[64];
    time(&t);

    sys_time = localtime(&t);

    strftime(buffer, 60, "%Y-%m-%d", sys_time);
    return buffer;
}