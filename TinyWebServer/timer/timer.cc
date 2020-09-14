#include "timer.h"
#include <signal.h>

void Timer::Init(int timeslot, int fd) {
    expire_ = time(nullptr) + 3 * timeslot;
    memset(&address_, 0, sizeof(address_));
    sockfd_ = fd;
}

bool Timer::operator<(const Timer &timer) const {
    return expire_ > timer.expire_;
}

int TimerContainer::epollfd_ = -1;
int TimerContainer::pipefd_ = -1;

void TimerContainer::Init(int timeslot) {
    timeslot_ = timeslot;
}

void TimerContainer::SigHandler(int sig) {
    /*可重入*/
    int saveerrno = errno;
    int msg = sig;
    write(pipefd_, (char*)&msg, 1);   //static_cast<char> error ?
    errno = saveerrno;
}

int TimerContainer::SetNonBlock(int fd) {
    int flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    flag = fcntl(fd, F_SETFL, flag);
    assert(flag == 0);
}

void TimerContainer::AddFd(int fd, bool one_shot, int trigmode) {
    epoll_event event; 
    event.data.fd = fd;
    event.events |= EPOLLIN;
    if(one_shot) event.events |= EPOLLONESHOT;
    if(trigmode == 1) event.events |= EPOLLET;
    epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event);
    SetNonBlock(fd);
}

void TimerContainer::AddSigHandler(int sig, void(handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SigHandler;
    if(restart) sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, nullptr) != -1);
}


void TimerContainer::TimerHandler() {
    alarm(timeslot_);
}

void TimerContainer::ShowError(int connfd, const char *info) {
    write(connfd, info, strlen(info));
    close(connfd);
}

void TimerContainer::UnMount(Timer &timer) {
    timer.ProcessExpire(timer.sockfd_);
}

void TimerContainer::AddTimer(Timer &timer) {
    timers_.insert({timer, timer.sockfd_});
}

bool TimerContainer::PopTimer(Timer &timer) {
    if(!timers_.count(timer)) return false;
    else timers_.erase(timer);
}


void ProcessExpire(int sockfd) {
    epoll_ctl(TimerContainer::epollfd_, EPOLL_CTL_DEL, sockfd, nullptr);
    close(sockfd);

    //HttpConnect::user_count_--; 主线程记得减少一个连接数量
    //这里不减少为了减少文件依赖
}