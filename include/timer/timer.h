#ifndef WEBSERVER_TIMER_H_
#define WEBSERVER_TIMER_H_

#include <time.h>
#include <memory>
#include <vector>
using namespace std;
class http_conn;

//定时器类
class heap_timer
{
public:
    heap_timer(int delay)
    {
        expire = time(NULL) + delay;
        hole_=-1;
        user_data.reset();
        cb_func==nullptr;
    }
    time_t expire;
    void (*cb_func)(shared_ptr<http_conn>);
    weak_ptr<http_conn> user_data;
    int hole_;
};

//时间堆类
class TimeHeap
{
public:
    TimeHeap(int cap) : capacity(cap), cur_size(0)
    {
        array.assign(cap,nullptr);
    }
    ~TimeHeap()
    {
        
    }
    bool add_timer(shared_ptr<heap_timer> timer)
    {
        if (!timer)
            return false;
        if (cur_size >= capacity - capacity / 20)
        {
            for (int i = 0; i < cur_size; i++)
            {
                if (NULL == array[i]->cb_func)
                {
                    array[i]->expire = 0;
                    percolate_up(i);
                }
            }
            tick();
            assert(cur_size < capacity);
        }
        int hole = cur_size++;
        array[hole] = timer;
        array[hole]->hole_ = hole;
        percolate_up(hole);
        return true;
    }
    bool del_timer(shared_ptr<heap_timer> timer)
    {
        if (!timer)
            return false;
        timer->cb_func = nullptr;
        timer->user_data.reset();
        return true;
    }
    shared_ptr<heap_timer> top() const
    {
        if (empty())
        {
            return nullptr;
        }
        return array[0];
    }
    void pop_timer()
    {
        if (empty())
        {
            return;
        }
        if (array[0])
        {
            array[0] = array[--cur_size];
            array[0]->hole_ = 0;
            percolate_down(0);
        }
    }
    void tick()
    {
        time_t cur = time(NULL);
        while (!empty())
        {
            if (!array[0])
            {
                pop_timer(); // TODO: break;
                continue;
            }
            if (array[0]->expire > cur)
            {
                break;
            }
            if (array[0]->cb_func)
            { 
                array[0]->cb_func(array[0]->user_data.lock());
            }
            pop_timer();
        }
    }
    bool adjustTimer(shared_ptr<heap_timer> timer, int delay)
    {
        if (!timer)
            return false;
        timer->expire = time(NULL) + delay;
        if (delay > 0)
        {
            percolate_down(timer->hole_);
        }
        else
        {
            percolate_up(timer->hole_);
        }
        return true;
    }
    bool empty() const { return cur_size == 0; }
    int size() const
    {
        return cur_size;
    }

private:
    void percolate_down(int hole)
    {
        shared_ptr<heap_timer> tmp = array[hole];
        int child;
        for (; (hole * 2 + 1) <= cur_size - 1; hole = child)
        {
            child = hole * 2 + 1;
            if (child < (cur_size - 1) && (array[child + 1]->expire < array[child]->expire))
            {
                ++child;
            }
            if (array[child]->expire < tmp->expire)
            {
                array[hole] = array[child];
                array[hole]->hole_ = hole;
            }
            else
            {
                break;
            }
        }
        array[hole] = tmp;
        array[hole]->hole_ = hole;
    }
    void percolate_up(int hole)
    {
        shared_ptr<heap_timer> tmp = array[hole];
        int parent = 0;
        for (; hole > 0; hole = parent)
        {
            parent = (hole - 1) / 2;
            if (array[parent]->expire <= tmp->expire)
            {
                break;
            }
            array[hole] = array[parent];
            array[hole]->hole_ = hole;
        }
        array[hole] = tmp;
        array[hole]->hole_ = hole;
    }
    vector<shared_ptr<heap_timer>> array;
    int capacity;
    int cur_size;
};
#endif //WEBSERVER_TIMER_H_