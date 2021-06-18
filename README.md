# WebServer
### 简介

> 本项目是一个在Linux下开发的高性能服务器，支持GET方法，可处理静态资源，支持POST方法，可注册登录。使用时间堆做定时处理，高效处理定时任务。
>
> + 使用**Epoll**边沿触发的**IO多路复用**技术，**非阻塞IO**，使用**Reactor模式**
>+ 使用**线程池**处理IO事件，避免线程频繁创建销毁的开销
> + 使用**状态机**解析HTTP请求报文，支持解析GET和POST请求
>+ 使用基于**小根堆的定时器**关闭超时请求
> + 使用**异步日志**避免频繁IO提升服务器性能
>+ 使用**配置文件**配置服务器
> 

### 模块介绍

+ [定时器](./docs/定时器.md)
+ [线程池](./docs/线程池.md)
+ [异步日志](docs/异步日志.md)
+ [数据库连接池](docs/数据库连接池.md)
+ [压力测试](docs/压力测试.md)

### 快速运行

+ 服务器测试环境
  + Ubuntu18.04
  + MySQL5.7.34
+ 测试前确认已安装MySQL数据库

``` shell
//shell下执行进入MySQL
mysql -u root -p
//执行 
source mydb.sql
//创建数据库与表
```

+ 修改config.ini中的修改配置信息

```ini
[common]
#服务器使用端口 默认1234
port=1234
#线程数量  默认5
thread_num=5 
#是否优雅关闭连接 默认0 关闭
linger=0
#请求队列中最大请求数量 默认 10000
max_request=10000

[mysql]
#主机名或IP地址
mysql_host=localhost
#数据库用户名
mysql_user=root
#数据库密码
mysql_passwd=matrix
#数据库名
mysql_db_name=mydb
#数据库端口号
mysql_port=3306
#数据库连接池中初始化连接数量 有效取值[1,1024]
mysql_conn_num=8

[logger]
#是否启用日志 默认启用
enable=1
#日志级别 0 DEBUG,1 INFO, 2 WARN, 3 ERROR 只输出大于等于该级别的日志信息
log_level=1
#日志文件名前缀
log_pre_filename=Serverlog
#日志缓冲区大小 会向上调整到1024的倍数
log_buf_size=65536
#单个日志文件的最大行数限制 粗略值
log_max_lines=1000000
```

+ build   &   运行webserver

```
sh  build.sh
./webserver
```

+ 参数简介
  + -p 端口号
  + -l 文件描述符linger选项是否开启(0 非0)  默认不开启 0 
  + -s 数据库连接池中连接数量  默认为8 
  + -t 线程数量 默认为8
  + -c 日志开关，默认打开
  + -r 请求队列最大长度
  + -h 查看参数说明

