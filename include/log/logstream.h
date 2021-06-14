#ifndef WEBSERVER_LOG_STREAM_H_
#define WEBSERVER_LOG_STREAM_H_

#include <string>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include "noncopyable.h"
#include "locker.h"
using namespace std;

const size_t MIN_BUF_SIZE = 4096;
const size_t MAX_BUF_SIZE = 4096 * 512;
/*  异步日志　logger 调用　LogStream　将日志写入到缓冲区
    核心思想：使用两个缓冲区，当一个缓冲区写满是，写另外一个缓冲，
            同时写入线程将写满的缓冲写入磁盘，不阻塞当前线程
            创建一个子进程用于当父进程错误退出是刷新缓冲区
            保证所有的日志持久化
*/
class LogStream : Noncopyable
{
public:
    LogStream();
    ~LogStream();
    //初始化日志缓冲　日志文件前缀名　缓冲区大小　日志文件最大行数
    bool init(string pre_filename, size_t buf_size, size_t max_lines);
    //写入日志文件到缓冲区
    int write(char *line, size_t len);
    //刷新缓冲区
    int flush();

private:
    //刷新缓冲线程执行函数
    void run();
    //将写满的缓冲刷新到磁盘
    int flushWriteBuf();
    //线程调用函数
    static void *writeToFile(void *arg);
    //获取当前时间　用于文件名
    string getTime();
    FILE *fp_;            //文件指针
    string pre_filename_; //文件前缀名
    string full_name_;    //文件全名
    string today_;        //当前具体时间
    int num_;             //当天日志文件数量　用于拼接文件名
    char *buf_;           //写入日志缓冲区　接受日志写入
    char *next_buf_;      //输出日志缓冲区　输出到文件
    char *write_buf_;     //备用日志缓冲区　
    size_t *buf_in_;      //当前日志待写入位置
    int *flag;            //当前buf使用的那个缓冲区
    size_t max_lines_;    //日志最大行数
    size_t buf_size_;     //日志缓冲区大小
    size_t cur_lines_;    //日志行数记录

    locker mutex;  //缓冲区互斥锁
    locker fmutex; //文件描述符互斥锁
    cond cond_;    //条件变量　用于唤醒线程
    bool stop;     //终止线程
};
#endif //WEBSERVER_LOG_STREAM_H_