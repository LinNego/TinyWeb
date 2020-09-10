#ifndef THREADPOOL_EVENT_EVENTINPUT_H_
#define THREADPOOL_EVENT_EVENTINPUT_H_
#include <string>
#include <queue>
#include <map>
#include <sys/epoll.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>

using std::queue;
using std::string;
using std::map;

const int kMAXSIZE = 4096;
const int kMAXFILELEN = 1024;

const char kMessage[8][100] = {
    "HTTP/1.1 200 OK",
    "HTTP/1.1 400 No",
    "HTTP/1.1 403 Forbidden",
    "HTTP/1.1 404 No resource",
    "HTTP/1.1 405 Method Forbidden",
    "HTTP/1.1 408 TimeOut",
    "HTTP/1.1 501 Incomplete",
    "HTTP/1.1 503 Server error"
};

class EventHttp {
private:
    /*输入的四个状态只有请求行是读取到\t\n就转移到下一个状态*/
    enum State {
        REQUEST,
        HEADER,
        COMPLETEHTTP,
    };
    enum LineState {
        COMPLETELINE,
        INCOMPLETELINE,
        BADLINE
    };
    enum Method {
        GET,
        POST,
        OTHER
    };
    enum HttpCode {
        Status200,       //OK
        Status400,       //不理解
        Status403,       //拒绝请求
        Status404,       //未找到资源
        Status405,       //方法禁用
        Status408,       //请求超时
        Status501,       //未实现该方法
        Status503,       //服务不可用
        Normal         //一切正常需要继续解析
    };
    enum WriteState {
        RESPONSELINE,
        RESPONSEHEADER,
        RESPONSEBODY
    };
    enum Pattern {
        ET,
        LT    
    };
public:
    /*将状态回复原样*/
    static int epollfd_;
    static int maxfd_;
    EventHttp(const EventHttp&) = delete;
    EventHttp& operator=(const EventHttp&) = delete;
    /*线程只管调用这个函数*/
    void Process();  //工作线程的调用函数
    int Read();  //主线程的函数
    bool Write();
    EventHttp(int sockfd) :sockfd_(sockfd){
        Initialize();
    }
    void CloseConnect();
    void Initialize();
/*需要个线程回调函数*/
private:
    int sockfd_;
    char *method_, *uri_, *version_; //Rem Release
    Method emethod_;
    char filepath_[kMAXFILELEN];
    char root_[kMAXFILELEN];
    State state_;

    char buf_[kMAXSIZE]; //服务器最多只能处理kMAXSIZE个字符的请求
    char *line_;
    int read_index_;      //一次读入的位置
    int check_index_;
    int line_index_;      //行开始的位置

    int content_length_;
    bool keep_alive_;
    char *host_;
    
    Pattern mode_;
    int num; 
    int in_fd;
    char writebuf_[kMAXSIZE];
    int write_index_;
    int write_state_;      //已经发送的位置
    int send_bytes_;
    struct stat file_stat_;

    int have_error_;


    void SendSignal();               
    //当状态达到了ENDING，将该sockfd push进全局队列，然后发信号告知主线程，主线程从就绪的的队列中将他们送入阻塞队列

    bool ProcessWrite(HttpCode);
    HttpCode PhaseRead();
    HttpCode PhaseHeader();
    HttpCode GetMethod();
    HttpCode GetUri();
    HttpCode GetVersion();
    HttpCode PhaseRequest();
    HttpCode PhaseComplete();
    HttpCode GetFile();
    HttpCode Do();

    LineState ReadLine();

    void WriteResponse(const char*, va_list);
    void WriteResponseLine(const char*, ...);
    void ClientError(const char*);


    
};
#endif /*THREADPOOL_EVENT_EVENTINPUT_H_*/
