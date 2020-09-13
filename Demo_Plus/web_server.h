#ifndef WEB_SERVER_H_
#define WEB_SERVER_H_
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include <string>
using std::string;

#include "http/http_event.h"
#include "timer/timer.h"
#include "sql_pool/sql_connection_pool.h"
#include "threadpool/threadpool.h"


const int kMAXFD = 65536;     //需要用超级用户权限修改
//const int kMAX_EVENT_NUMBER = 10000;
const int kMAX_EVENT_NUMBER = 20000;
const int TIMESLOT = 5;

class WebServer {
public:
    WebServer(); 
    ~WebServer();
    /*port, user, password, databasename, log_write, opt_linger, trigmode, sql_num, thread_num, close_log, actor_model*/
    void Init(int, string, string, string, int, int, int, int , int, int, int);

    void ThreadPools();
    void SqlPool();
    void LogWrite();
    void TrigMode();
    void EventListen();
    void EventLoop();
    void BuildTimer(int, struct sockaddr_in);
    void AdjustTimer(Timer&);
    void DeleteTimer(Timer&);
    bool AcceptClient();
    bool DealWithSignal(bool&, bool&);
    void DealWithRead(int);
    void DealWithWrite(int); 

private:
    int port_;
    char *root_;
    int log_write_;
    int close_log_;
    int actormodel_;

    int pipefd_[2];
    int epollfd_;
    HttpEvent *http_events_;    

    SqlConnectionPool *sqlpool_;
    string user_;
    string password_;
    string databasename_;
    int sql_num_;

    ThreadPool *threadpool_;
    int thread_num_;

    //epoll_event events[kMAX_EVENT_NUMBER];
    epoll_event *events;

    int listenfd_;
    int opt_linger_;
    int trig_mode_;
    int listen_trigmode_;
    int conn_trigmode_;

    Timer *user_timer_;
    TimerContainer timers_;
};
#endif