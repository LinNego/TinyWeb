#include "operate_fd.h"

int SetNonBlock(int fd) {
    int flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flag)) return -1;
    return 0;
}

int Register(int epollfd, int fd, int event, int cmd) {
    struct epoll_event epoll_events;
    epoll_events.data.fd = fd;
    epoll_events.events = event;
    int ret;
    if(cmd == 0) ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &epoll_events);
    else    ret = epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &epoll_events);
    return ret;

}
int UnRoll(int epollfd, int fd) {
    int ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, nullptr);
    return ret;
}