#include "event_http.h"
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <stdarg.h>
#include "operate_fd.h"

int EventHttp::epollfd_ = -1;
int EventHttp::maxfd_ = -1;
void EventHttp::Initialize() {
    method_ = uri_ = version_ = nullptr;
    line_ = nullptr;
    memset(filepath_, 0, sizeof(filepath_));
    strncpy(root_, "./root", 7);
    memset(buf_, 0, sizeof(buf_)); /*  性能*/
    memset(writebuf_, 0, sizeof(writebuf_));
    read_index_ = check_index_ = line_index_ = 0;
    state_ = REQUEST;
    mode_ = ET;
    write_index_ = write_state_ = send_bytes_ = 0;
    memset(&file_stat_, 0, sizeof(file_stat_));
    SetNonBlock(sockfd_);
    Register(epollfd_, sockfd_, EPOLLIN | EPOLLONESHOT | EPOLLHUP, 0);
    have_error_ = true;
    num = 0;
} 

void EventHttp::CloseConnect() {
    UnRoll(epollfd_, sockfd_);
    int ret = close(sockfd_);
    if(ret != 0) {
        printf("cause %s\n", strerror(errno));
    }
        
    ret = close(in_fd);
    if(ret != 0) {
        printf("case %s\n", strerror(errno));
    }
}

/*主线程调用函数*/
/*ET模式，需要把所有的输入都读进来*/
/*如果缓冲区已满，需要返回一个值给主线程*/
int EventHttp::Read() {
    if(read_index_ >= kMAXSIZE) return -1;
    for(; ;) {
        int nread = read(sockfd_, buf_ + read_index_, kMAXSIZE - read_index_);
        if(nread < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break;      //读完了
            }
        }
        else if(nread == 0) return -1;
        read_index_ += nread;

    }
    return 0;
}


/*逻辑处理，工作线程*/
EventHttp::LineState EventHttp::ReadLine() {
    for(; check_index_ < read_index_; ) {
        if(buf_[check_index_] == '\r') {
            if(check_index_ + 1 < read_index_ && buf_[check_index_ + 1] != '\n') {
                return BADLINE;
            }
            else if(check_index_ + 1 >= read_index_) {
                return INCOMPLETELINE;
            }
            else if(check_index_ + 1 < read_index_ && buf_[check_index_ + 1] == '\n') {
                line_ = buf_ + line_index_;
                line_index_ = check_index_ + 2;
                if(check_index_ != read_index_ - 2)
                    buf_[check_index_] = buf_[check_index_ + 1] = 0; 
                return COMPLETELINE;
            }
        }
        else if(buf_[check_index_] == '\n') {
                printf("check %d\n", check_index_);
                if(check_index_ >= 1 && buf_[check_index_ - 1] != '\r') {
                    return BADLINE;
                }
                else if(buf_[check_index_ - 1] == '\t') {
                    line_ = buf_ + line_index_;
                    line_index_ = check_index_ + 1;
                    if(check_index_ != line_index_)
                        buf_[check_index_] = buf_[check_index_ - 1] = 0;
                    return COMPLETELINE;
                }
                else 
                    return COMPLETELINE;
        } 
        ++check_index_;
    }
    return INCOMPLETELINE;
}

EventHttp::HttpCode EventHttp::GetFile() {
    int ret;
    ret = stat(filepath_, &file_stat_);
    if(ret != 0) {
        return Status404;
    }
    if(!S_ISREG(file_stat_.st_mode) || !(file_stat_.st_mode & S_IRUSR)) {
        return Status403;
    }
    int fd = open(filepath_, O_RDONLY);
    printf("fd%d\n", fd);
    if(fd < 0) {
        return Status404;
    }
    in_fd = fd;
    return Normal;
}
EventHttp::HttpCode
EventHttp::GetMethod() {
    if(strcasecmp(method_, "GET") == 0) {
        emethod_ = GET;
    }
    else if(strcasecmp(method_, "POST") == 0) {
        emethod_ = POST; 
    }
    else {
        return Status503;
    }
    return Normal;
}

EventHttp::HttpCode
EventHttp::GetUri() {
    HttpCode ret;
    if(!strcasecmp(uri_, "/")) {
        strcpy(filepath_, root_);
        strcat(filepath_, "/home.html");
    }
    else if(!strcasecmp(uri_, "/home.html")) {
        strcpy(filepath_, root_);
        strcat(filepath_, uri_);
    }
    else {
        //strcpy(filepath_, root_);
        //strcat(filepath_, uri_);
        return Status400;
    }
    ret = GetFile();
    return ret;
}

EventHttp::HttpCode
EventHttp::GetVersion() {
    if(strcasecmp(version_, "HTTP/1.1") && strcasecmp(version_, "HTTP/1.0")) {
        return Status400;
    }
    return Normal;
}

EventHttp::HttpCode 
EventHttp::PhaseRequest() {
    state_ = HEADER;
    HttpCode ret;
    char *temp = line_, *c;
    c = strchr(temp, ' ');
    *c = 0;
    method_ = temp;
    temp = c + 1;
    c = strchr(temp, ' ');
    *c = 0;
    uri_ = temp;
    temp = c + 1;
    version_ = temp;
    ret = GetMethod();
    if(ret != Normal) {
        //printf("method error\n");
        return ret;
    }

    ret = GetUri();
    if(ret != Normal) {
        //printf("uri_ error\n");
        return ret;
    }
    
    ret = GetVersion();
    return ret;
}

EventHttp::HttpCode
EventHttp::PhaseHeader() {
    if(strcasecmp(line_, "\r\n") == 0) {
        state_ = COMPLETEHTTP; 
        return Status200;
    }
    else if(strncasecmp(line_, "Content-length: ", 16) == 0) {
        line_ += 16;
        content_length_ = atol(line_);
    }
    else if(strncasecmp(line_, "Connection: ", 12) == 0) {
        line_ += 12;
        keep_alive_ = true;
    }
    else if(strncasecmp(line_, "Host: ", 6) == 0) {
        line_ += 6;
        host_ = line_;   
    }
    return Normal;
}

void EventHttp::WriteResponse(const char *format, va_list ap) {
    int len = vsnprintf(writebuf_ + write_index_, kMAXSIZE - 1 - write_index_, format, ap);
    write_index_ += len;
    return ;
}

void EventHttp::WriteResponseLine(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    WriteResponse(format, ap);
    va_end(ap);
}

void EventHttp::ClientError(const char *msg) {
    int len = sprintf(writebuf_ + write_index_, "%s\r\n", msg);
    write_index_ += len; 
    len = sprintf(writebuf_ + write_index_, "Content-type: text/html\r\n\r\n");
    write_index_ += len;

    len = sprintf(writebuf_ + write_index_, "<html><title>Tiny Error</title>");
    write_index_ += len;
    len = sprintf(writebuf_ + write_index_, "%s\r\n", msg);
    write_index_ += len;
    len = sprintf(writebuf_ + write_index_, "<hr><em>The Tiny Web server </em> \r\n");
    write_index_ += len;

}

EventHttp::HttpCode EventHttp::PhaseRead() {
    HttpCode ret;
    LineState linestate;
    while((linestate = ReadLine()) == COMPLETELINE) {
        num += strlen(line_);
        switch(state_) {
            case REQUEST:
                ret = PhaseRequest();
                if(ret != Normal) {
                    ClientError(kMessage[ret]);
                    state_ = COMPLETEHTTP;
                    return ret;
                }
                break;
            case HEADER:
                ret = PhaseHeader();
                if(ret != Normal) {
                    return ret;
                }
                break;
        }
    }
    if(linestate == BADLINE) {
        ClientError(kMessage[1]);
        return Status400;
    }
    return Normal;
}

bool EventHttp::Write() {
    printf("sockfd %d\n", sockfd_);
    int ret;
    if(write_state_ < write_index_) {
        for(; ;) {
            ret = write(sockfd_, writebuf_ + write_state_, strlen(writebuf_ + write_state_));
            if(ret < 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    Register(epollfd_, sockfd_, EPOLLOUT | EPOLLONESHOT, 1);
                    return true;
                }
                else {
                    printf("write\n");
                    CloseConnect();
                    return false;
                }
            }
            write_state_ += ret;
            if(write_state_ >= write_index_) {
                if(have_error_) CloseConnect();
                else break;
            }
        }
    }
    if(write_state_ >= write_index_) {
        for(; ;) {
            ret = sendfile(sockfd_, in_fd, nullptr, 4096); 
            if(ret < 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    Register(epollfd_, sockfd_, EPOLLOUT | EPOLLONESHOT, 1);
                    return true;;
                }
                else {
                    printf("errno %s\n", strerror(errno));
                    CloseConnect();
                    return false;
                }
            }
            send_bytes_ += ret;
            if(send_bytes_ >= file_stat_.st_size) {
                CloseConnect();
                break;
            }
        }
    }
    return true;
}


void EventHttp::Process() {
    HttpCode ret;
    if(state_ != COMPLETEHTTP) {
        ret = PhaseRead();
        if(ret == Status200) {
            WriteResponseLine("%s\r\n", kMessage[0]);
            WriteResponseLine("Content-type: text/html\r\n");
            WriteResponseLine("Content-length: %lld\r\n\r\n", file_stat_.st_size);
            Register(epollfd_, sockfd_, EPOLLOUT | EPOLLONESHOT, 1);
        }
        else if(have_error_) {
            Register(epollfd_, sockfd_, EPOLLOUT | EPOLLONESHOT, 1);
        }
    }
    else {
        /*for reactor*/
    }
}



