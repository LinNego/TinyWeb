#ifndef TIMER_H_
#define TIMER_H_

#include <queue>
#include <map>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>
#include <cstdlib>
#include <cassert>
#include <errno.h>
#include <cstring>
#include <unistd.h>
#include <ctime>


class Timer {
public:
    void Init(int timeslot, int sockfd); 
    bool operator<(const Timer &timer) const;
    void (*ProcessExpire)(int sockfd);  //是否需要用回调
    time_t expire_;
    sockaddr_in address_;
    int sockfd_;
};


/*这是个装载定时器容器的类*/
/*底层数据结构用优先队列, 所以需要重载定时器类的小于号　< */
/*这个容器类主要负责调用alarm()，信号处理向主线程发送通过管道发送信号*/
/*主线程从这个容器类中pop出定时器进行处理*/

class TimerContainer {
public:

    void Init(int timeslot);
        
    int SetNonBlock(int fd);

    void AddFd(int fd, bool one_shot, int trigmode);

    static void SigHandler(int sig);

    void AddSigHandler(int sig, void(handler)(int), bool restart = true);

    void TimerHandler();

    void ShowError(int connfd, const char *info);

    void UnMount(Timer&);

    void AddTimer(Timer &timer);


    bool PopTimer(Timer &timer); 

    static int epollfd_;   
    static int pipefd_;    //管道写入端的文件描述符，主线程持有读取端
private:
    int sockfd_;            //主监视
    int timeslot_;
    //std::priority_queue<Timer*> timers;
    std::map<Timer, int> timers_;
};

void ProcessExpire(int sockfd);

#endif /*end of timer.h*/