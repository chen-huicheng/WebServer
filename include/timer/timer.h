#ifndef WEBSERVER_TIMER_H_
#define WEBSERVER_TIMER_H_

#include <time.h>
class http_conn;

//定时器类
class heap_timer
{
public:
    heap_timer(int delay)
    {
        expire = time(NULL) + delay;
    }
    time_t expire;
    void (*cb_func)(http_conn *);
    http_conn *user_data;
    int hole;
};

//时间堆类
class TimeHeap
{
public:
    TimeHeap(int cap) : capacity(cap), cur_size(0)
    {
        array = new heap_timer *[capacity];
        if (!array)
        {
            throw std::exception();
        }
        for (int i = 0; i < capacity; ++i)
        {
            array[i] = NULL;
        }
    }
    ~TimeHeap()
    {
        for (int i = 0; i < cur_size; ++i)
        {
            delete array[i];
        }
        delete[] array;
    }
    bool add_timer(heap_timer *timer)
    {
        if (!timer)
            return false;
        if (cur_size >= capacity - capacity / 20)
        {
            for (int i = 0; i < cur_size; i++)
            {
                if (array[i]->cb_func == NULL)
                {
                    array[i]->expire = 0;
                    percolate_up(timer->hole);
                }
            }
            // printf("before: %d:%d\n",cur_size,capacity);
            tick();
            // printf("end   : %d:%d\n",cur_size,capacity);
            assert(cur_size < capacity);
        }
        int hole = cur_size++;
        array[hole] = timer;
        array[hole]->hole = hole;
        percolate_up(hole);
        return true;
    }
    bool del_timer(heap_timer *timer)
    {
        if (!timer)
            return false;
        timer->cb_func = NULL;
        timer->user_data = NULL;
        return true;
    }
    heap_timer *top() const
    {
        if (empty())
        {
            return NULL;
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
            delete array[0];
            array[0] = array[--cur_size];
            array[0]->hole = 0;
            // if(cur_size)
            //     array[cur_size]=NULL;
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
            { //&&array[0]->user_data){//TODO: array[0]->cb_func  ==null  array[0]->user_data!=null
                array[0]->cb_func(array[0]->user_data);
            }
            pop_timer();
        }
    }
    bool adjustTimer(heap_timer *timer, int delay)
    {
        if (!timer)
            return false;
        timer->expire = time(NULL) + delay;
        if (delay > 0)
        {
            percolate_down(timer->hole);
        }
        else
        {
            percolate_up(timer->hole);
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
        heap_timer *tmp = array[hole];
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
                array[hole]->hole = hole;
            }
            else
            {
                break;
            }
        }
        array[hole] = tmp;
        array[hole]->hole = hole;
    }
    void percolate_up(int hole)
    {
        heap_timer *tmp = array[hole];
        int parent = 0;
        for (; hole > 0; hole = parent)
        {
            parent = (hole - 1) / 2;
            if (array[parent]->expire <= tmp->expire)
            {
                break;
            }
            array[hole] = array[parent];
            array[hole]->hole = hole;
        }
        array[hole] = tmp;
        array[hole]->hole = hole;
    }
    heap_timer **array;
    int capacity;
    int cur_size;
};
#endif //WEBSERVER_TIMER_H_