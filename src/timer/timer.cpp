#include "timer.h"

bool TimeHeap::add_timer(shared_ptr<heap_timer> timer) //添加一个定时器
{
    if (!timer)
        return false;
    if (cur_size >= capacity) //定时堆当前容量大于最大容量   将调整一次堆
    {
        tick();
        assert(cur_size < capacity);
    }
    int hole = cur_size++;
    array[hole] = timer;
    array[hole]->hole_ = hole;
    percolate_up(hole);
    return true;
}
bool TimeHeap::del_timer(shared_ptr<heap_timer> timer) //延迟销毁
{
    if (!timer)
        return false;
    mutex.lock();
    overtimer.push_back(timer->hole_);
    mutex.unlock();
    timer->cb_func = nullptr;
    timer->user_data.reset();
    return true;
}
void TimeHeap::pop_timer() //删除堆顶元素 智能指针直接覆盖就行 不用手动销毁
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
void TimeHeap::tick() //删除超时定时器  心搏函数
{
    time_t cur = time(NULL);
    adjust();
    while (!empty())
    {
        if (!array[0])
        {
            pop_timer();
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
bool TimeHeap::adjustTimer(shared_ptr<heap_timer> timer, int delay)
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

void TimeHeap::percolate_down(int hole)
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
void TimeHeap::percolate_up(int hole)
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
void TimeHeap::adjust()
{
    mutex.lock();
    vector<int>tmp(overtimer);
    overtimer.clear();
    mutex.unlock();
    for (auto i:tmp)
    {
        if (NULL == array[i]->cb_func)
        {
            array[i]->expire = 0;
            percolate_up(i);
        }
    }
}

// shared_ptr<TimeHeap>  Timer::timer;
