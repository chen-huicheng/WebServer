#include "config.h"
#include "util.h"
#include <fstream>
#include <iostream>
Config::Config()
{
    //端口号,默认1234
    port = 1234;

    //优雅关闭链接，默认不使用
    linger = 0;

    //线程池内的线程数量,默认5
    thread_num = 5;

    //请求队列中最大请求数量
    max_request = 10000;

    //数据库连接池数量,默认8
    mysql_conn_num = 8;
    mysql_port=3306;


    //关闭日志,默认不关闭
    close_log = 0;
    log_level=1;
    log_pre_filename="ServerLog";
    log_buf_size=65536;
    log_max_lines=1000000;
}

void Config::parse_arg(int argc, char *argv[])
{
    int opt;
    const char *str = "p:l:s:t:cr:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            port = atoi(optarg);
            break;
        }
        case 'l':
        {
            linger = atoi(optarg);
            break;
        }
        case 's':
        {
            mysql_conn_num = atoi(optarg);
            break;
        }
        case 't':
        {
            thread_num = atoi(optarg);
            break;
        }
        case 'c':
        {
            close_log = true;
            break;
        }
        case 'r':
        {
            max_request = atoi(optarg);
            break;
        }
        default:
            usage();
            abort();
            break;
        }
    }
}
void Config::parse_ini_file(string filename){
    ifstream infile(filename.c_str());
	if (!infile) 
	{
		fprintf(stderr, "file open error!");
		return;
	}
    map<string,string> kv;
    string line,key,val;
    while (getline(infile, line)){
        if(line.size()==0||line[0]=='#'||line[0]=='[')
            continue;
        if(parse_line(line,key,val))
            kv[key]=val;
    }
    if(kv.find("port")!=kv.end()){
        port=stoi(kv["port"]);
    }
    if(kv.find("thread_num")!=kv.end()){
        thread_num=stoi(kv["thread_num"]);
    }
    if(kv.find("linger")!=kv.end()){
        linger=stoi(kv["linger"]);
    }
    if(kv.find("max_request")!=kv.end()){
        max_request=stoi(kv["max_request"]);
    }
    if(kv.find("mysql_host")!=kv.end()){
        mysql_host=kv["mysql_host"];
    }
    if(kv.find("mysql_user")!=kv.end()){
        mysql_user=kv["mysql_user"];
    }
    if(kv.find("mysql_passwd")!=kv.end()){
        mysql_passwd=kv["mysql_passwd"];
    }
    if(kv.find("mysql_db_name")!=kv.end()){
        mysql_db_name=kv["mysql_db_name"];
    }
    if(kv.find("mysql_port")!=kv.end()){
        mysql_port=stoi(kv["mysql_port"]);
    }
    if(kv.find("mysql_conn_num")!=kv.end()){
        mysql_conn_num=stoi(kv["mysql_conn_num"]);
    }
    if(kv.find("enable")!=kv.end()){
        if(stoi(kv["enable"])==0){
            close_log=1;
        }
    }
    if(kv.find("log_level")!=kv.end()){
        log_level=stoi(kv["log_level"]);
    }
    if(kv.find("log_pre_filename")!=kv.end()){
        log_pre_filename=kv["log_pre_filename"];
    }
    if(kv.find("log_buf_size")!=kv.end()){
        log_buf_size=stoi(kv["log_buf_size"]);
    }
    if(kv.find("log_max_lines")!=kv.end()){
        log_max_lines=stoi(kv["log_max_lines"]);
    }

}
bool Config::parse_line(string line,string &key,string &val){
    if (line.empty())
	{
		return false;
	}
	int start_pos = 0, end_pos = line.size() - 1, pos = 0;
	if ((pos = line.find('#')) != -1)
	{
		if (0 == pos)
		{//行的第一个字符就是注释字符
			return false;
		}
		end_pos = pos - 1;
	}
	string new_line = line.substr(start_pos, end_pos + 1 - start_pos);  // 预处理，删除注释部分
 
	if ((pos = new_line.find('=')) == -1)
	{
		return false;  // 没有=号
	}
 
	key = new_line.substr(0, pos);
	val = new_line.substr(pos + 1, end_pos + 1- (pos + 1));
 
	trim(key);
	if (key.empty())
	{
		return false;
	}
	trim(val);
	return true;
}
void Config::trim(string &str){
    if (str.empty())
	{
		return;
	}
	size_t i, start_pos, end_pos;
	for (i = 0; i < str.size(); ++i)
	{
		if (!isspace(str[i]))
		{
			break;
		}
	}
	if (i == str.size())
	{
		str = "";
		return;
	}
 
	start_pos = i;
 
	for (i = str.size() - 1; i >= 0; --i)
	{
		if (!isspace(str[i]))
		{
			break;
		}
	}
	end_pos = i;
 
	str = str.substr(start_pos, end_pos - start_pos + 1);
}
