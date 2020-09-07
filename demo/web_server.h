#ifndef SERVER_WEB_SERVER_H_
#define SERVER_WEB_SERVER_H_
#include "threadpool/synchronous.h"
#include "threadpool/threadpool.h"
#include "threadpool/event_http.h"
#include <map>
#include <utility>
#include <memory>
#include <exception>
#include "paramater/paramater.h"
#include <sys/socket.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>
#include <ctime>
#include <csignal>
#include <csetjmp>

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const int kMAXEVENT = 65535;
const int kMAXFD = 65535;

class WebServer {
public:
    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;
    void LogPool();
    void EventListen();
    void EventLoop();
    WebServer(Paramater &paramater) {
        max_connect_ = paramater.max_connect_;
        thread_num_ = paramater.thread_num_;        
        EventHttp::maxfd_ = paramater.max_connect_;
        port_ = atoi(paramater.port_str_.c_str());
        addr_str_ = paramater.addr_str_;
        port_str_ = paramater.port_str_;
        name_ = "Tiny Server";
        threadpool_ptr_ = std::make_shared<ThreadPool>(thread_num_, 500);
    }

private:
    int max_connect_; //允许的最大客户数量
    int epoll_fd_; //建立监听的连接
    int mode_;     //Server默认为ET(边缘触发)
    std::map<int, std::shared_ptr<EventHttp>> fd_events_;
   //监听的描述符对应的http的事件

    epoll_event evlist_[1000];
    epoll_event ev_;

    std::shared_ptr<ThreadPool> threadpool_ptr_;
    int thread_num_;

    uint16_t port_;
    std::string addr_str_;
    std::string port_str_;
    std::string name_; //log

    int listen_fd_;
    void OpenListenFd();
    bool SetNonBlock(int);
    void Assert(int exp, std::string cause) {
        if(exp < 0) throw std::runtime_error(cause);
    }
    void AcceptClient();

};

#endif
