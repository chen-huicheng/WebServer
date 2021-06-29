#ifndef WEBSERVER_SESSION_H_
#define WEBSERVER_SESSION_H_
#include <openssl/md5.h>
#include <time.h>
#include <string>
#include <map>
#include <list>

#include "locker.h"
using namespace std;
const int TIMEOUT = 30; //单位分钟
enum USER_STATUS        //用户状态
{
    LOGIN = 0,
    UNLOGIN
};
struct User //用户信息
{
    string username;    //用户名
    string sessionid;   //sessionId
    USER_STATUS status; //状态
    timt_t last_time;   //上次请求时间
};
class Session
{
public:
    Session(int capacity, int timeout) : cap(capacity), timeout(timeout) //Session最大容量
    {
    }
    USER_STATUS get_status(string sessionid)
    {
        MutexLockGuard lock(mutex);
        if (cache.count(sessionid))
        {
            movetotop(sessionid);
            return cache[sessionid]->status;
        }
        return UNLOGIN;
    }
    string get_username(string sessionid)
    {
        MutexLockGuard lock(mutex);
        if (cache.count(sessionid))
        {
            movetotop(sessionid);
            return cache[sessionid]->username;
        }
        return "";
    }
    int put(string username, string sessionid) //首次添加返回sessionid
    {
        MutexLockGuard lock(mutex);
        if (0 == cache.count(sessionid))
        {
            if (users.size() >= cap)
            {
                cache.erase(users.back().sessionid);
                users.pop_back();
            }
            User user;
            user.username = username;
            user.sessionid = sessionid;
            user.last_time = time(NULL);
            user.status = LOGIN;
            users.push_front(user);
            cache[sessionid] = users.begin();
            dealtimeout();
            return 0;
        }
        return -1;
    }

private:
    void movetotop(string key)
    {
        auto p = *cache[key];
        users.erase(cache[key]);
        users.push_front(p);
        cache[key] = users.begin();
        dealtimeout();
    }
    void dealtimeout()
    {
        time_t curtime = time(NULL);
        while (users.begin() != users.end() && users.end()->last_time + timeout * 60 < curtime)
        {
            cache.erase(users.back().sessionid);
            users.pop_back();
        }
    }
    typedef list<User> List;
    int cap;                           // Cache容量
    int timeout;                       //session失效时间
    List users;                        //访问时间排序
    map<string, List::iterator> cache; //缓冲
    locker mutex;
};

#endif //WEBSERVER_SESSION_H_