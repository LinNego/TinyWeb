/*这个类是对互斥量进行加锁*/
/*利用了RAII手法，构造时对互斥量lock，析构时对互斥量unlock*/
/*构造时接受一个Mutex互斥量*/

#ifndef SYNCHRONOUS_MUTEX_MUTEXLOCK_H_
#define SYNCHRONOUS_MUTEX_MUTEXLOCK_H_
#include "mutex.h"
class MutexLock {
public:
    MutexLock(Mutex &mutex);
    ~MutexLock();
private:
    Mutex &mutex_;
};

#endif  /*SYNCHRONOUS_MUTEX_MUTEX_LOCK_H_*/