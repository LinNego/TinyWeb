#include "synchronous.h"
#include <semaphore.h>
/*将同步放到一个文件中定义是为了编译方便*/

/*ConditionVarible的定义*/
ConditionVarible::ConditionVarible() { 
    pthread_cond_init(&cond_, nullptr);
}
ConditionVarible::~ConditionVarible() { 
    pthread_cond_destroy(&cond_);
}

void ConditionVarible::Notify() {
    if(pthread_cond_signal(&cond_) != 0) {
        throw std::runtime_error("条件变量唤醒单一线程失败");
    }
}
void ConditionVarible::NotifyAll() {
    if(pthread_cond_broadcast(&cond_) != 0) {
        throw std::runtime_error("条件变量通知全部线程失败");
    }
}

bool ConditionVarible::Wait(pthread_mutex_t *mutex_) {
    int ret = pthread_cond_wait(&cond_, mutex_);
    return ret == 0;
}


/*Mutex的定义*/
Mutex::Mutex() {
    if(pthread_mutex_init(&mutex_, nullptr) != 0) {  //默认的方式取锁
        throw std::runtime_error("初始化互斥量错误");
    }
}
Mutex::~Mutex() {
    pthread_mutex_destroy(&mutex_);
}

void Mutex::Lock() {
    if(pthread_mutex_lock(&mutex_) != 0) {
        throw std::runtime_error("对互斥量加锁错误");
    }
}

void Mutex::Unlock() {
    if(pthread_mutex_unlock(&mutex_) != 0) {
        throw std::runtime_error("对互斥量解锁错误");
    }
}

pthread_mutex_t* Mutex::GetMutexAddr() { return &mutex_;}

/*MutexLock*/
MutexLock::MutexLock(Mutex &mutex):mutex_(mutex) {mutex_.Lock(); }
MutexLock::~MutexLock() {mutex_.Unlock(); }

/*Sem*/

Sem::Sem(int num) { 
    if(sem_init(&sem_t_, 0, num) != 0) throw std::runtime_error("初始化信号量错误");
}

Sem::~Sem() {
    sem_destroy(&sem_t_);
}

void Sem::Wait() {
    int ret;
    while((ret = sem_wait(&sem_t_)) != 0) {
        if(ret == EINTR) continue;
        else throw std::runtime_error("等待信号量错误");
    }
}

void Sem::Post() {
    if(sem_post(&sem_t_) != 0) throw std::runtime_error("递增信号量错误");
}