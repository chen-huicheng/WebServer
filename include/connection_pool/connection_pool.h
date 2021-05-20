#pragma once

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string>
#include "locker.h"
#define MIN_CONN_NUM 3  //数据库最少链接数

class ConnectionPool{
public:
    static ConnectionPool* GetInstance();
    void init(std::string url,std::string user,std::string password,std::string db_name,int port ,int max_conn);
    MYSQL *GetConnection();				 //获取数据库连接
	bool ReleaseConnection(MYSQL *conn); //释放连接
    int GetMaxConn(){
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

    std::string host_;		//主机地址
	std::string user_;		 //登陆数据库用户名
	std::string passwd_;	 //登陆数据库密码
	std::string db_name_; //使用数据库名
    int port_;		    //数据库端口号
};
class Connection
{
public:
    MYSQL *GetConn()const{
        return conn_;
    }
    Connection();
    ~Connection();
private:
    Connection(Connection&);
    Connection &operator=(Connection&);
    MYSQL *conn_;
};
