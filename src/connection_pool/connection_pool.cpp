#include "connection_pool.h"

ConnectionPool::ConnectionPool() : reserve_(0)
{
    max_conn_ = 0;
    free_conn_ = 0;
    is_init_ = false;
}

ConnectionPool::~ConnectionPool()
{
    lock_.lock();
    for (auto it : pool_)
    {
        if (NULL == it)
            continue;
        mysql_close(it);
    }
    pool_.clear();
    lock_.unlock();
}

ConnectionPool *ConnectionPool::GetInstance()
{
    static ConnectionPool connPool;
    return &connPool;
}

void ConnectionPool::init(std::string host, std::string user, std::string passwd, std::string db_name, int port, int max_conn)
{
    if (is_init_)
    {
        LOG_WARN("Connection pool has been initialized\n");
        return;
    }

    is_init_ = true;
    host_ = host;
    user_ = user;
    port_ = port;
    passwd_ = passwd;
    db_name_ = db_name;
    max_conn_ = std::max(max_conn, MIN_CONN_NUM);
    max_conn_=std::min(max_conn_,MAX_CONN_NUM);

    for (int i = 0; i < max_conn_; i++)
    {
        MYSQL *conn = NULL;
        conn = mysql_init(conn);
        if (NULL == conn)
        {
            // log
            LOG_ERROR("mysqlpool init failed!!!\n");
            abort();
        }
        conn = mysql_real_connect(conn, host.c_str(), user.c_str(), passwd.c_str(), db_name.c_str(), port, NULL, 0);
        if (NULL == conn)
        {
            LOG_ERROR("mysqlpool init failed!!!\n");
            abort();
        }
        pool_.push_back(conn);
        reserve_.post();
        free_conn_++;
    }

    max_conn_ = free_conn_;
}

MYSQL *ConnectionPool::GetConnection()
{
    MYSQL *conn = NULL;

    reserve_.wait();
    lock_.lock();
    if (0 == pool_.size())
    {
        lock_.unlock();
        return conn;
    }
    conn = pool_.front();
    pool_.pop_front();
    lock_.unlock();
    return conn;
}
bool ConnectionPool::ReleaseConnection(MYSQL *conn)
{
    if (NULL == conn)
    {
        return false;
    }
    lock_.lock();
    pool_.push_back(conn);
    lock_.unlock();
    reserve_.post();
    conn = NULL;
    return true;
}

Connection::Connection()
{
    conn_ = ConnectionPool::GetInstance()->GetConnection();
}

Connection::~Connection()
{
    ConnectionPool::GetInstance()->ReleaseConnection(conn_);
}
