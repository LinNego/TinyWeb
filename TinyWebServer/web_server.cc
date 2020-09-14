#include "web_server.h"

WebServer::WebServer() {
    //HttpEvent类对象
    http_events_ = new HttpEvent[kMAXFD];
    //root文件夹路径
    char server_path[200];
    getcwd(server_path, 200);
    char root[7] = "/root";
    root_ = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(root_, server_path);
    strcat(root_, root);
    timers_.Init(TIMESLOT);
    //定时器
    user_timer_ = new Timer[kMAXFD];
}

WebServer::~WebServer() {
    close(epollfd_);
    close(listenfd_);
    close(pipefd_[1]);
    close(pipefd_[0]);
    delete[] http_events_;
    delete[] user_timer_;
    delete threadpool_;
}

void WebServer::Init(int port, string user, string passWord, string databaseName, int log_write, 
                     int opt_linger, int trigmode, int sql_num, int thread_num, int close_log, int actor_model)
{
    port_ = port;
    user_ = user;
    password_ = passWord;
    databasename_ = databaseName;
    sql_num_ = sql_num;
    thread_num_ = thread_num;
    log_write_ = log_write;
    opt_linger_ = opt_linger;
    trig_mode_ = trigmode;
    close_log_ = close_log;
    actormodel_ = actor_model;
    events = new epoll_event[kMAX_EVENT_NUMBER];
}

void WebServer::TrigMode()
{
    //LT + LT
    if (0 == trig_mode_)
    {
        listen_trigmode_ = 0;
        conn_trigmode_ = 0; 
    }
    //LT + ET
    else if (1 == trig_mode_)
    {
        listen_trigmode_ = 0;
        conn_trigmode_ = 1;
    }
    //ET + LT
    else if (2 == trig_mode_)
    {
        listen_trigmode_ = 1;
        conn_trigmode_ = 0;
    }
    //ET + ET
    else if (3 == trig_mode_)
    {
        listen_trigmode_ = 1;
        conn_trigmode_ = 1;
    }
}

void WebServer::LogWrite()
{
    if (0 == close_log_)
    {
        //初始化日志
        if (1 == log_write_)     //异步
            Log::GetInstance()->Init("./ServerLog", 800000, close_log_, 0);
        else
            Log::GetInstance()->Init("./ServerLog", 800000, close_log_, 1);
    }
}

void WebServer::SqlPool()
{
    //初始化数据库连接池
    sqlpool_ = SqlConnectionPool::GetInstance();
    sqlpool_->Init(user_, password_, databasename_, 3306, sql_num_);

    //初始化数据库读取表
    http_events_->InitmysqlResult(sqlpool_);
}

void WebServer::ThreadPools()
{
    //线程池
    threadpool_ = new ThreadPool(actormodel_, sqlpool_, thread_num_);
}

void WebServer::EventListen()
{
    //网络编程基础步骤
    listenfd_ = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd_ >= 0);

    //优雅关闭连接
    if (0 == opt_linger_)
    {
        struct linger tmp = {0, 1};
        setsockopt(listenfd_, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }
    else if (1 == opt_linger_)
    {
        struct linger tmp = {1, 1};
        setsockopt(listenfd_, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port_);

    int flag = 1;
    setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(listenfd_, (struct sockaddr *)&address, sizeof(address));
    if(ret != 0)  {
        printf("bind error %s\n", strerror(errno));
    }
    assert(ret >= 0);
    ret = listen(listenfd_, 5);
    if(ret != 0) {
        printf("listen error\n");
    }
    assert(ret >= 0);

    timers_.Init(TIMESLOT);

    //epoll创建内核事件表
    //epoll_event events[MAX_EVENT_NUMBER];
    epollfd_ = epoll_create(5);
    assert(epollfd_ != -1);

    TimerContainer::epollfd_ = epollfd_;
    timers_.AddFd(listenfd_, false, listen_trigmode_);
    HttpEvent::epollfd_ = epollfd_;
    

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd_);
    assert(ret != -1);
    timers_.SetNonBlock(pipefd_[1]);
    timers_.AddFd(pipefd_[0], false, 0);
    timers_.AddSigHandler(SIGPIPE, SIG_IGN);
    timers_.AddSigHandler(SIGALRM, timers_.SigHandler, false);
    timers_.AddSigHandler(SIGTERM, timers_.SigHandler, false);

    alarm(TIMESLOT);

    //工具类,信号和描述符基础操作
    TimerContainer::pipefd_ = pipefd_[1];
}

void WebServer::BuildTimer(int connfd, struct sockaddr_in client_address)
{
    http_events_[connfd].Init(connfd, client_address, root_, conn_trigmode_, close_log_, user_, password_, databasename_);

    //初始化client_data数据
    //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
    user_timer_[connfd].address_ = client_address;
    user_timer_[connfd].sockfd_ = connfd;
    time_t cur = time(nullptr);
    user_timer_[connfd].ProcessExpire = ProcessExpire;
    user_timer_[connfd].expire_ = 3 * TIMESLOT + cur;
    timers_.AddTimer(user_timer_[connfd]);
}

//若有数据传输，则将定时器往后延迟3个单位
//并对新的定时器在链表上的位置进行调整
void WebServer::AdjustTimer(Timer &timer)
{
    Timer temp = timer; 
    timers_.PopTimer(temp);
    time_t cur = time(nullptr);
    timer.expire_ = cur + 3 * TIMESLOT;
    timers_.AddTimer(timer);

    LOG_INFO("%s", "adjust timer once");
}

void WebServer::DeleteTimer(Timer &timer)
{
    timer.ProcessExpire(timer.sockfd_);
    timers_.PopTimer(timer);
    HttpEvent::user_count_--;

    LOG_INFO("close fd %d", timer.sockfd_);
}

bool WebServer::AcceptClient()
{
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    if (0 == listen_trigmode_)
    {
        int connfd = accept(listenfd_, (struct sockaddr *)&client_address, &client_addrlength);
        if (connfd < 0)
        {
            LOG_ERROR("%s:errno is:%d", "accept error", errno);
            return false;
        }
        if (HttpEvent::user_count_ >= kMAXFD)
        {
            timers_.ShowError(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        BuildTimer(connfd, client_address);
    }

    else
    {
        while (1)
        {
            int connfd = accept(listenfd_, (struct sockaddr *)&client_address, &client_addrlength);
            if (connfd < 0)
            {
                LOG_ERROR("%s:errno is:%d", "accept error", errno);
                break;
            }
            if (HttpEvent::user_count_ >= kMAXFD)
            {
                timers_.ShowError(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                break;
            }
            BuildTimer(connfd, client_address);
        }
        return false;
    }
    return true;
}

bool WebServer::DealWithSignal(bool &timeout, bool &stop_server)
{
    int ret = 0;
    int sig;
    char signals[1024];
    ret = recv(pipefd_[0], signals, sizeof(signals), 0);
    if (ret == -1)
    {
        return false;
    }
    else if (ret == 0)
    {
        return false;
    }
    else
    {
        for (int i = 0; i < ret; ++i)
        {
            switch (signals[i])
            {
            case SIGALRM:
            {
                timeout = true;
                break;
            }
            case SIGTERM:
            {
                stop_server = true;
                break;
            }
            }
        }
    }
    return true;
}

void WebServer::DealWithRead(int sockfd)
{
    Timer &timer = user_timer_[sockfd];

    //reactor
    if (1 == actormodel_)
    {
        AdjustTimer(timer);
        //若监测到读事件，将该事件放入请求队列
        threadpool_->ReactorPush(http_events_ + sockfd, 0);

        while (true)
        {
            if (1 == http_events_[sockfd].improv_)
            {
                if (1 == http_events_[sockfd].timer_flag_)
                {
                    DeleteTimer(timer);
                    http_events_[sockfd].timer_flag_ = 0;
                }
                http_events_[sockfd].improv_ = 0;
                break;
            }
        }
    }
    else
    {
        //proactor
        if (http_events_[sockfd].ReadOnce())
        {
            //printf("proactor\n");
            LOG_INFO("deal with the client(%s)", inet_ntoa(http_events_[sockfd].GetAddress()->sin_addr));
            //printf("LOG有问题？\n");
            //若监测到读事件，将该事件放入请求队列
            threadpool_->Push(http_events_ + sockfd);

           AdjustTimer(timer);
        }
        else
        {
            DeleteTimer(timer);
        }
    }
}

void WebServer::DealWithWrite(int sockfd)
{
    Timer timer = user_timer_[sockfd];
    //reactor
    if (1 == actormodel_)
    {
        AdjustTimer(timer);
        threadpool_->ReactorPush(http_events_ + sockfd, 1);

        while (true)
        {
            if (1 == http_events_[sockfd].improv_)
            {
                if (1 == http_events_[sockfd].timer_flag_)
                {
                    DeleteTimer(timer);
                    http_events_[sockfd].timer_flag_ = 0;
                }
                http_events_[sockfd].improv_ = 0;
                break;
            }
        }
    }
    else
    {
        //proactor
        if (http_events_[sockfd].Write())
        {
            LOG_INFO("send data to the client(%s)", inet_ntoa(http_events_[sockfd].GetAddress()->sin_addr));
            AdjustTimer(timer);
        }
        else
        {
            DeleteTimer(timer);
        }
    }
}

void WebServer::EventLoop()
{
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server)
    {
        int number = epoll_wait(epollfd_, events, kMAX_EVENT_NUMBER, -1);
        //printf("%d\n", number);
        if (number < 0 && errno != EINTR)
        {
            //printf("%s\n", strerror(errno));
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;

            //处理新到的客户连接
            if (sockfd == listenfd_)
            {
                //printf("new connect\n");
                bool flag = AcceptClient();
                if (false == flag)
                    continue;
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                //printf("关闭连接\n");
                //服务器端关闭连接，移除对应的定时器
                Timer &timer = user_timer_[sockfd];
                DeleteTimer(timer);
            }
            //处理信号
            else if ((sockfd == pipefd_[0]) && (events[i].events & EPOLLIN))
            {
                //printf("处理信号\n");
                bool flag = DealWithSignal(timeout, stop_server);
                if (false == flag)
                    LOG_ERROR("%s", "dealclientdata failure");
            }
            //处理客户连接上接收到的数据
            else if (events[i].events & EPOLLIN)
            {
                //printf("可读\n");
                DealWithRead(sockfd);
            }
            else if (events[i].events & EPOLLOUT)
            {
                //printf("可写\n");
                DealWithWrite(sockfd);
            }
        }
        if (timeout)
        {
            timers_.TimerHandler();

            LOG_INFO("%s", "timer tick");

            timeout = false;
        }
    }
}
