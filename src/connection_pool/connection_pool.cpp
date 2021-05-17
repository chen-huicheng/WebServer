#include "connection_pool.h"

ConnectionPool::ConnectionPool(){
    max_conn_=0;
    free_conn_=0;
}
ConnectionPool::~ConnectionPool(){
    lock_.lock();
    for(auto it:pool_){
        mysql_close(it);
    }
    pool_.clear();
    lock_.unlock();
}

ConnectionPool* ConnectionPool::GetInstance(){
    static ConnectionPool connPool;
    return &connPool;
}

void ConnectionPool::init(std::string host,std::string user,std::string passwd,std::string db_name,int port ,int max_conn,int close_log){
    host_ = host;
	user_ = user;
    port_ = port;
    passwd_ = passwd;
    db_name_=db_name;
    max_conn_=max_conn;
    close_log_=close_log;
    for(int i=0;i<max_conn_;i++){
        MYSQL *conn;
        conn=mysql_init(conn);
        if(conn == NULL){
            // log
            throw std::exception();
        }
        conn = mysql_real_connect(conn,host.c_str(),user.c_str(),passwd.c_str(),db_name.c_str(),port,NULL,0);
        if(conn ==NULL){
            throw std::exception();
        }
        pool_.push_back(conn);
        ++free_conn_;
    }
    reserve_ = sem(free_conn_);

    max_conn_ = free_conn_;
}

MYSQL* ConnectionPool::GetConnection(){
    MYSQL *conn = NULL;

	reserve_.wait();
	lock_.lock();
    if(0==pool_.size()){
        lock_.unlock();
        return conn;
    }
	conn = pool_.front();
	pool_.pop_front();
	lock_.unlock();
	return conn;
}
bool ConnectionPool::ReleaseConnection(MYSQL *conn){
    if(NULL == conn){
        return false;
    }
    lock_.lock();
    pool_.push_back(conn);
    ++free_conn_;
    lock_.unlock();
    reserve_.post();
    return true;
}


Connection::Connection(){
    conn_=ConnectionPool::GetInstance()->GetConnection();
}

Connection::Connection(){
    ConnectionPool::GetInstance()->ReleaseConnection(conn_);
}
