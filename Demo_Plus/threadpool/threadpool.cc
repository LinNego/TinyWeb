#include "threadpool.h"


ThreadPool::ThreadPool( int actor_model, SqlConnectionPool *sqlpool, int thread_number, int max_requests) : actor_model_(actor_model),thread_num_(thread_number), 
                                        max_requests_(max_requests), threads_(nullptr), sqlpool_(sqlpool) {
    if (thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    threads_ = new pthread_t[thread_num_];
    if (!threads_)
        throw std::exception();
    for (int i = 0; i < thread_number; ++i)
    {
        if (pthread_create(threads_ + i, NULL, Worker, this) != 0)
        {
            delete[] threads_;
            throw std::exception();
        }
        if (pthread_detach(threads_[i]))
        {
            delete[] threads_;
            throw std::exception();
        }
    }
}
ThreadPool::~ThreadPool() {
    delete[] threads_;
}

bool ThreadPool::ReactorPush(HttpEvent* request, int state) {
    MutexLock mutexlock(queuelocker_);
    if (work_queue_.size() >= max_requests_)
    {
        return false;
    }
    request->state_ = state;
    work_queue_.push(request);
    queuestat_.Post();
    return true;
}
bool ThreadPool::Push(HttpEvent *request)
{
    //printf("Push\n");
    MutexLock mutexlock(queuelocker_);
    if (work_queue_.size() >= max_requests_)
    {
        return false;
    }
    work_queue_.push(request);
    queuestat_.Post();
    return true;
}
void *ThreadPool::Worker(void *arg) {
    ThreadPool *pool = (ThreadPool*)arg;
    pool->Run();
    return pool;
}

void ThreadPool::Run() {
    while (true) {
        //printf("pre Run\n");
        queuestat_.Wait();
        //printf("next Run\n");
        queuelocker_.Lock();
        if (work_queue_.empty()) {
            queuelocker_.Unlock();
            continue;
        }
        HttpEvent *request = work_queue_.front();
        work_queue_.pop();
        queuelocker_.Unlock();
        if (!request)
            continue;
        if (actor_model_ == 1) {
            if (0 == request->state_) {
                if (request->ReadOnce()) {
                    request->improv_ = 1;
                    Sql mysqlcon(&request->mysql_, sqlpool_);
                    request->Process();
                }
                else {
                    request->improv_ = 1;
                    request->timer_flag_ = 1;
                }
            }
            else {
                if (request->Write()) {
                    request->improv_ = 1;
                }
                else {
                    request->improv_ = 1;
                    request->timer_flag_ = 1;
                }
            }
        }
        else {
            //printf("工作了\n");
            Sql mysqlcon(&request->mysql_, sqlpool_);
            request->Process();
        }
    }
}