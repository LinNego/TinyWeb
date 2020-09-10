#include "web_server.h"
#include <cstring>
#include <exception>
#include <assert.h>
#include <sys/types.h>
#include <utility>
#include "threadpool/operate_fd.h"

extern const int MAXEVENT = 65535;


bool WebServer::SetNonBlock(int fd) {
    int flag;
    flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flag) != 0) return false;
    return true;
}
void WebServer::OpenListenFd() {
    int ret;
    if(addr_str_.empty()) {
        addrinfo *result, hints;
        memset(&hints, 0, sizeof(hints)); 
        hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        ret = getaddrinfo(nullptr, port_str_.c_str(), &hints, &result);
        Assert(ret >= 0, "getaddrinfo error");
        bool flag = 0;
        int ret, optval = 1;
        while(result != nullptr) {
            listen_fd_ = socket(result->ai_family, result->ai_socktype, result->ai_protocol);  
            if(listen_fd_ == -1) {
                result = result->ai_next;
                continue;
            }
            int optval = 1;
            setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR,
                        static_cast<const void*>(&optval), sizeof(int));
            linger linger = {1, 1}; 
            setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER,
                        static_cast<const void*>(&linger), sizeof(linger));
            
            ret = bind(listen_fd_, result->ai_addr, result->ai_addrlen);
            if(ret != 0) {
                result = result->ai_next; 
                continue;
            } 
            ret = listen(listen_fd_, max_connect_);
            if(ret != 0) {
                result = result->ai_next;
            } 
            flag = 1;
            break;
        }
        Assert(flag, "建立监听失败");
    }
    else {
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        ret = inet_pton(AF_INET, addr_str_.c_str(), &addr.sin_addr);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        int optval = 1;
        ret = setsockopt(listen_fd_, SOL_SOCKET, SO_RCVBUFFORCE, 
                (const void*)&optval, sizeof(int));

        ret = bind(listen_fd_, (sockaddr*)&addr, sizeof(addr));
        Assert(ret == 0, "bind error");
        ret = listen(listen_fd_, max_connect_);
        Assert(ret == 0, "listen error");
    }
}


void WebServer::EventListen() {
    int ret;
    OpenListenFd();
    ret = SetNonBlock(listen_fd_); 
    Assert(ret, "设置阻塞失败");
    epoll_fd_ = epoll_create(5);
    EventHttp::epollfd_ = epoll_fd_;
    Assert(epoll_fd_ >= 0, "epoll_create失败");
    ev_.events = EPOLLIN | EPOLLONESHOT;
    ev_.data.fd = listen_fd_;
    ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev_);
    Assert(ret == 0, "epoll_ctl失败");
    EventHttp::epollfd_ = epoll_fd_;
}

void WebServer::EventLoop() {
    bool stop_server = false;
    while(!stop_server) {
        int ready_number = epoll_wait(epoll_fd_, evlist_, 200, -1);
        if(ready_number < 0) {
            if(errno != EINTR) {
                printf("%s\n", strerror(errno));
                throw std::runtime_error("致命错误epoll_wait");
            }
            else {continue;}
        }

        for(int i = 0; i < ready_number; ++i) {
            int sockfd = evlist_[i].data.fd;
            if(sockfd == listen_fd_) {
               // printf("new connect\n");
                AcceptClient(); 
            }
            else if(evlist_[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
                fd_events_[sockfd]->CloseConnect();
                fd_events_.erase(sockfd); 
            }
            /*可写*/
            else if(evlist_[i].events & EPOLLOUT){
                fd_events_[sockfd]->Write();
            }
            /*可读*/
            else if(evlist_[i].events & EPOLLIN) {
                fd_events_[sockfd]->Read();
                threadpool_ptr_->Push(fd_events_[sockfd].get());
            }
        }
    }
}


void WebServer::AcceptClient() {
    sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    int connfd;
    for(; ;) {
        memset(&cliaddr, 0, sizeof(cliaddr));
        connfd = accept(listen_fd_, (sockaddr*)&cliaddr, &clilen);
        if(connfd < 0) {
            break;
        }
        fd_events_.insert({connfd, std::make_shared<EventHttp>(connfd)});
        Register(epoll_fd_, listen_fd_, EPOLLIN | EPOLLONESHOT, 1);
    }
    return ;
}

