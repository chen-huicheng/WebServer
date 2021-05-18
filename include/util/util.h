#pragma once

#include <stdlib.h>
#include <string>

ssize_t readn(int fd, void *buff, size_t n);
ssize_t readline(int fd,void *buf,size_t maxlen);
ssize_t writen(int fd, void *buff, size_t n);

int setnonblocking(int fd);
void addfd(int epollfd,int fd,bool one_shot);
void removefd(int epollfd, int fd);
void modfd(int epollfd, int fd, int ev);
int open_listenfd(int port);