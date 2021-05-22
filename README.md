# WebServer
### 简介

> 本项目是一个在Linux下开发的高性能服务器，支持GET方法，可处理静态资源，支持POST方法，可注册登录。使用时间堆做定时处理，高效处理定时任务。
>
> + 使用**Epoll**边沿触发的**IO多路复用**技术，**非阻塞IO**，使用**Reactor模式**
> + 使用**线程池**处理IO事件，避免线程频繁创建销毁的开销
> + 使用**状态机**解析HTTP请求报文，支持解析GET和POST请求
> + 使用基于**小根堆的定时器**关闭超时请求

### 快速运行

+ 服务器测试环境
  + Ubuntu18.04
  + MySQL5.7.34
+ 测试前确认已安装MySQL数据库

``` 
// 建立yourdb库
create database yourdb;

// 创建user表
USE yourdb;
CREATE TABLE user1(
    username char(50) NOT NULL UNIQUE,
    passwd char(50) NULL
);

// 添加数据
INSERT INTO user(username, passwd) VALUES('name', 'passwd');
```

+ 修改main.cpp中的数据库初始化信息

```
    //需要修改的数据库信息,登录名,密码,库名
    string user = "root";
    string passwd = "123456";
    string db_name = "yourdb";
```

+ build   &   运行webserver

```
sh  build.sh
./webserver
```

+ 参数简介
  + -p 端口号
  + -l  文件描述符linger选项是否开启(0 非0)  默认不开启 0 
  + -s  数据库连接池中连接数量  默认为8 
  + -t 线程数量 默认为8
  + -c 日志开关，默认打开   0:打开  1:关闭

