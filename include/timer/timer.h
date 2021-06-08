#ifndef WEBSERVER_TIMER_H_
#define WEBSERVER_TIMER_H_

#include <time.h>
#include <memory>
#include <vector>
#include <cassert>
#include "locker.h"
using namespace std;
class http_conn;

/*
    定时器类
    与http_conn类使用weak_ptr相互指向
    避免shared_ptr循环引用造成无法释放
    cb_func  ：定时器超时时调用的函数
    expire   ：定时器的到期时间  绝对时间
    user_data：指向定时器归属与那个用户
    hole_    ：用于在时间堆中调制
*/
class heap_timer
{
public:
    heap_timer(int delay)
    {
        expire = time(NULL) + delay;
        hole_ = -1;
        user_data.reset();
        cb_func = nullptr;
    }
    time_t expire;
    void (*cb_func)(shared_ptr<http_conn>);
    weak_ptr<http_conn> user_data;
    int hole_;
};

/*
    时间堆类
    以vector为基础数据机构实现的一个时间堆
    删除使用延迟删除策略，删除时仅是将定时器中的cb_func

    线程不安全  
*/
class TimeHeap
{
public:
    TimeHeap(int cap) : capacity(cap), cur_size(0) //初始化
    {
        array.assign(cap, nullptr);
    }
    bool add_timer(shared_ptr<heap_timer> timer); //添加一个定时器
    bool del_timer(shared_ptr<heap_timer> timer);  //延迟销毁
    shared_ptr<heap_timer> top() const //获得堆顶元素的指针
    {
        if (empty())
        {
            return nullptr;
        }
        return array[0];
    }
    void pop_timer();   //删除堆顶元素 智能指针直接覆盖就行 不用手动销毁
    void tick();        //心搏函数
    bool adjustTimer(shared_ptr<heap_timer> timer, int delay);//调制时间  相对的向后向前延迟
    bool empty() const { return cur_size == 0; }
    int size() const {return cur_size;}

private:
    void percolate_down(int hole);//下虑操作
    void percolate_up(int hole);//上虑操作
    vector<int> overtimer;
    locker mutex;
    void adjust(); //调整堆  将已删除的节点进行上虑
    vector<shared_ptr<heap_timer>> array;
    int capacity;
    int cur_size;
};
#endif //WEBSERVER_TIMER_H_