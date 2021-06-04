#ifndef WEBSERVER_THREAD_POOL_H_
#define WEBSERVER_THREAD_POOL_H_

#include <pthread.h>
#include <list>
#include <exception>
#include <cstdio>
#include <vector>

#include "logger.h"
#include "locker.h"

using namespace std;
const int MIN_THREAD_num = 1;
const int MAX_THREAD_NUM = 1024;

template <typename T>
class threadpool
{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(int thread_number = 5, int max_request = 10000);
    ~threadpool();
    bool append(shared_ptr<T> request);

private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void *worker(void *arg);
    void run();

private:
    int thread_number_;                  //线程池中的线程数
    uint max_requests_;                  //请求队列中允许的最大请求数
    vector<pthread_t> m_threads_;        //描述线程池的数组，其大小为m_thread_number
    std::list<shared_ptr<T>> workqueue_; //请求队列  TODO:多个队列实现
    locker queuelocker_;                 //保护请求队列的互斥锁
    sem queuestat_;                      //是否有任务需要处理
    bool stop_;                          //是否结束线程
};
template <typename T>
threadpool<T>::threadpool(int thread_number, int max_requests) : thread_number_(thread_number), max_requests_(max_requests), stop_(false)
{
    if (thread_number <= 0 || max_requests <= 0)
    {
        LOG_ERROR("threadpool init exception!!!\n");
        throw std::exception();
    }
    thread_number_ = min(thread_number_, MAX_THREAD_NUM);
    for (int i = 0; i < thread_number; ++i)
    {
        pthread_t tid;
        if (pthread_create(&tid, NULL, worker, this) != 0)
        {
            throw std::exception();
        }
        if (pthread_detach(tid))
        {
            throw std::exception();
        }
        m_threads_.push_back(tid);
    }
}
template <typename T>
threadpool<T>::~threadpool()
{
    stop_ = true;
}

template <typename T>
bool threadpool<T>::append(shared_ptr<T> request)
{
    if (workqueue_.size() >= max_requests_)
    {
        return false;
    }
    queuelocker_.lock();
    workqueue_.push_back(request);
    queuelocker_.unlock();
    queuestat_.post();
    return true;
}
template <typename T>
void *threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}
template <typename T>
void threadpool<T>::run()
{
    while (!stop_)
    {
        queuestat_.wait();
        queuelocker_.lock();
        if (workqueue_.empty())
        {
            queuelocker_.unlock();
            continue;
        }
        shared_ptr<T> request = workqueue_.front();
        workqueue_.pop_front();
        queuelocker_.unlock();
        if (!request)
            continue;
        request->run();
    }
}
#endif //WEBSERVER_THREAD_POOL_H_