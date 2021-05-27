#include <string>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "noncopyable.h"
#include "locker.h"
using namespace std;

class LogStream : Noncopyable
{
public:
    LogStream();
    ~LogStream();
    void init(string pre_filename, size_t buf_size, size_t max_lines);
    int write(char *line, int len);
    int flush();

private:
    void run();
    int flushWriteBuf();
    static void *writeToFile(void *arg);
    string getTime();
    FILE *fp_;
    string pre_filename_; //路径名
    string full_name_;
    string today_;
    int num_;
    char *buf_;
    char *next_buf_;
    char *write_buf_;
    size_t buf_in_;    //当前日志待写入位置
    size_t buf_out_;   //可输出位置
    size_t max_lines_; //日志最大行数
    size_t lines_;
    size_t buf_size_;  //日志缓冲区大小
    size_t cur_lines_; //日志行数记录

    locker mutex;
    locker fmutex;
    cond cond_;
    bool stop;
};