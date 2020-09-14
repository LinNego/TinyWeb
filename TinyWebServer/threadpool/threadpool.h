#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <queue>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../synchronous.h"
#include "../sql_pool/sql_connection_pool.h"
#include "../http/http_event.h"

class ThreadPool
{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    ThreadPool(int actor_model, SqlConnectionPool *connPool, int thread_number = 8, int max_request = 10000);
    ~ThreadPool();
    bool ReactorPush(HttpEvent*, int state);
    bool Push(HttpEvent*);

private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void *Worker(void *arg);
    void Run();

private:
    int thread_num_;        //线程池中的线程数
    int max_requests_;         //请求队列中允许的最大请求数
    pthread_t *threads_;       //描述线程池的数组，其大小为m_thread_number
    std::queue<HttpEvent*> work_queue_;
    Mutex queuelocker_;       //请求队列的互斥锁
    Sem queuestat_;            //是否有任务需要处理
    SqlConnectionPool *sqlpool_;  //数据库
    int actor_model_;          //模型切换   reactor或proactor
};

#endif