#ifndef OPERATE_FD_H_
#define OPERATE_FD_H_
#include <fcntl.h>
#include <sys/epoll.h>
int SetNonBlock(int fd);
int Register(int epollfd, int fd, int event, int cmd);
int UnRoll(int epollfd, int fd);
#endif /*OPERATE_FD_H_*/