#include "threadpool.h"
#include <stdexcept>

void* ThreadPool::Worker(void *arg) {
    ThreadPool *threadpool = static_cast<ThreadPool*>(arg);
    threadpool->Run();
    return threadpool;
}

ThreadPool::ThreadPool(int thread_num, int max_request): thread_num_(thread_num), 
        max_request_(max_request), threads_(nullptr){
    if(thread_num_ <= 0 || max_request_ <= 0) {
        throw std::runtime_error("线程数或者事件请求太多"); 
    }
    threads_ = new pthread_t[thread_num_];
    if(!threads_) {
        throw std::runtime_error("分配线程数失败");
    }
    for(int i = 0; i < thread_num_; ++i) {
        if(pthread_create(threads_ + i, nullptr, Worker, this) != 0) {
            delete []threads_;
            throw std::runtime_error("初始化线程失败");
        }
        if(pthread_detach(threads_[i])) {
            delete []threads_;
            throw std::runtime_error("分离线程失败");
        }
    }
}

bool ThreadPool::Push(EventHttp *eventhttp) {
    MutexLock mutexlock(mutex_);
    if(block_queue_.size() > max_request_) {
        return false;
    }
    block_queue_.push(eventhttp);
    sem_.Post();
    return true;
}

void ThreadPool::Run() {
    for(; ;) {
        sem_.Wait();
        mutex_.Lock();
        EventHttp *eventhttp = block_queue_.front();
        block_queue_.pop();
        mutex_.Unlock();
        eventhttp->Process();
    }
}
