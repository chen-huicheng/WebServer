#ifndef WEBSERVER_LOCKER_H_
#define WEBSERVER_LOCKER_H_
#include <exception>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "locker.h"
#include "noncopyable.h"
/* 封装信号量的类 */
class sem:private Noncopyable
{
public:
    //初始化一个为0的信号量
    sem()
    {
        if (sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception();
        }
    }
    //初始化一个为value的信号量
    sem(int value)
    {
        if (sem_init(&m_sem, 0, value) != 0)
        {
            throw std::exception();
        }
    }
    ~sem()
    {
        sem_destroy(&m_sem);
    }
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }
    bool trywait()
    {
        return sem_trywait(&m_sem) == 0;
    }
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};

/* 封装互斥锁的类 */
class locker:private Noncopyable
{
public:
    locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool trylock()
    { //如果目标可以加锁，对目标加锁，不能加锁返回false
        return pthread_mutex_trylock(&m_mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

private:
    pthread_mutex_t m_mutex;
};

#endif //WEBSERVER_LOCKER_H_