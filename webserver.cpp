#include "locker.h"
#include <pthread.h>
#include "connection_pool.h"
int main(){
    Connection conn;
    if(mysql_query(conn.GetConn(),"select * from user")<=0){
        printf("ERR\n");
    }else{
        printf("SUCCESS\n");
    }

}