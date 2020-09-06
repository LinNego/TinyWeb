#ifndef THREADPOOL_THREADPOOL_H_
#define THREADPOOL_THREADPOOL_H_
#include "synchronous.h"
#include "event_http.h"
#include <queue>
#include <pthread.h>

/*
class BlockQueue {
public:
    EventHttp* Front() {
        return queue_.front(); 
    } 
    void Pop() {
        sem_.Wait();
        queue_.pop();
    }
    void Push(EventHttp *eventhttp) {
        sem_.Post();
        queue_.push(eventhttp);
    }
private:
    std::queue<EventHttp*> queue_;
    Mutex mutex_;
    Sem sem_;
    int cap_;
};
*/

class ThreadPool {
friend class EventHttp;
public:
    ThreadPool(int thread_num, int max_request);
    ~ThreadPool() { delete []threads_; }
    bool Push(EventHttp*);
private:
    static void* Worker(void *arg); //线程的回调函数
    void Run();
private:
    int thread_num_;
    int max_request_;
    pthread_t *threads_;
    std::queue<EventHttp*> block_queue_;
    Sem sem_;
    Mutex mutex_;
    void MakeThread();
    EventHttp* Front();

};


#endif /*THREADPOOL_THREADPOOL_H_*/