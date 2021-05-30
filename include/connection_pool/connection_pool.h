#ifndef WEBSERVER_CONNECTION_POOL_H_
#define WEBSERVER_CONNECTION_POOL_H_

#include <cstdio>
#include <list>
#include <mysql/mysql.h>
#include <string>
#include <algorithm>

#include "locker.h"
#include "noncopyable.h"
#include "logger.h"
const int MIN_CONN_NUM = 3; //数据库最少链接数

class ConnectionPool
{
public:
    //单例模式 
    static ConnectionPool *GetInstance();
    //初始化数据库连接  url:数据库地址  user:用户名  passwd:密码  db_name:数据库名  port:端口号  max_conn:在连接池中初始化多少连接
    void init(std::string url, std::string user, std::string passwd, std::string db_name, int port, int max_conn);
    //获取数据库连接
    MYSQL *GetConnection();
    //释放连接
    bool ReleaseConnection(MYSQL *conn); 
    int GetMaxConn()
    {
        return max_conn_;
    }

private:
    ConnectionPool();
    ~ConnectionPool();

    int max_conn_;  //最大连接数
    int free_conn_; //当前空闲的连接数
    locker lock_;
    std::list<MYSQL *> pool_; //连接池
    sem reserve_;

    std::string host_;    //主机地址
    std::string user_;    //登陆数据库用户名
    std::string passwd_;  //登陆数据库密码
    std::string db_name_; //使用数据库名
    int port_;            //数据库端口号
    bool is_init_;        //是否初始化   仅能初始化一次  因此只能链接一个数据库
};

//使用 Connection 类来管理连接 
//构建函数 从连接池中获取连接
//析构函数 释放连接到连接池
class Connection : Noncopyable
{
public:
    //供用户使用的借口
    MYSQL *GetConn() const
    {
        return conn_;
    }
    Connection();
    ~Connection();

private:
    MYSQL *conn_;
};
#endif //WEBSERVER_CONNECTION_POOL_H_