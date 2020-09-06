/*这是一个互斥量的类, 利用RAII封装对互斥量初始化，销毁的过程*/
#ifndef SYNCHRONOUS_MUTEX_MUTEX_H_
#define SYNCHRONOUS_MUTEX_MUTEX_H_
#include <pthread.h>
#include <stdexcept>
class Mutex {
public:
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;
    Mutex();
    ~Mutex();

    void Lock();
    void Unlock();
    pthread_mutex_t* GetMutexAddr();
private:
    pthread_mutex_t mutex_;
    //pthread_t tid_;  //想法基于muduo,虽然暂时还不知道保存锁的持有者有啥用*/
};
#endif  /*SYNCHRONOUS_MUTEX_MUTEX_H_*/
/*
#ifndef SYNCHRONOUS_MUTEX_MUTEX_H_
#define SYNCHRONOUS_MUTEX_MUTEX_H_
#include "mutex.h"
class MutexLock {
public:
    MutexLock(Mutex &mutex):mutex_(mutex) {mutex_.lock(); }
    ~MutexLock() {mutex_.unlock(); }
private:
    Mutex &mutex_;
};
*/