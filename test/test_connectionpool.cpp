#include <stdio.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <vector>
#include "connection_pool.h"

#include  <time.h> 
#define MAX_BUF_LEN 100
using namespace std;

void *insert_by_default(void *){
    MYSQL *conn;
    int res;
    conn = mysql_init(conn);
    if(conn==NULL){
        printf("error\n");
        return;
    }
    char buf[MAX_BUF_LEN];
    if (mysql_real_connect(conn, "localhost", "root", "matrix", "mydb", 0, NULL, CLIENT_FOUND_ROWS))
    {
        // printf("connect success!\n");
        int id = syscall(SYS_gettid);
        sprintf(buf,"insert into user values('%d','%d')",id,id);
        res = mysql_query(conn, buf);
        if (res)
        {
            printf("error\n");
        }
        // else
        // {
        //     printf("OK\n");
        // }
        mysql_close(conn);
    }
    return NULL;
}
void *insert_by_conn_pool(void *){
    Connection conn;
    char buf[MAX_BUF_LEN];
    int id = syscall(SYS_gettid);
    sprintf(buf,"insert into user values('%d','%d')",id,id);
    int res = mysql_query(conn.GetConn(), buf);
    if (res)
    {
        printf("error\n");
    }
    return NULL;
}
void test_insert(int n_pid){
    vector<pthread_t> pids;
    clock_t st = clock();
    for(int i=0;i<n_pid;i++){
        pthread_t pid;
        pthread_create(&pid,NULL,insert_by_default,NULL);
        pids.push_back(pid);
    }
    for(auto pid:pids){
        pthread_join(pid,NULL);
    }
    clock_t et = clock();
    double duration = (double)(et-st)/CLOCKS_PER_SEC;
    printf("insert %d rows using %f s\n",n_pid,duration);
 
}
void test_insert_by_conn_pool(int n_pid){
    vector<pthread_t> pids;
    clock_t st = clock();
    for(int i=0;i<n_pid;i++){
        pthread_t pid;
        pthread_create(&pid,NULL,insert_by_conn_pool,NULL);
        pids.push_back(pid);
    }
    for(auto pid:pids){
        pthread_join(pid,NULL);
    }
    clock_t et = clock();
    double duration = (double)(et-st)/CLOCKS_PER_SEC;
    printf("insert_by_conn_pool %d rows using %f s\n",n_pid,duration);
}
int main(int argc, char *argv[])
{
    
    if(argc<2){
        printf("Usage:%s num_of_thread",argv[0]);
    }
    int n_pid=atoi(argv[1]);
    ConnectionPool::GetInstance()->init("localhost", "root", "matrix", "mydb",0,8);
    
    test_insert(n_pid);

    test_insert_by_conn_pool(n_pid);

    return 0;
}