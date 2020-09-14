/*这个是对条件变量进行封装的类*/
#ifndef SYNCHRONOUS_CONDITION_VARIBLE__CONDITION_VARIBLE_H_
#define SYNCHRONOUS_CONDITION_VARIBLE_CONDITION_VARIBLE_H_
#include <pthread.h>
#include <stdexcept>

class ConditionVarible {
public:
    ConditionVarible(const ConditionVarible&) = delete;
    ConditionVarible& operator=(const ConditionVarible&) = delete;
    ConditionVarible();
    ~ConditionVarible();
    void Notify();
    void NotifyAll();
    bool Wait(pthread_mutex_t *mutex_);

private:
    pthread_cond_t cond_;
};

#endif /*SYNCHRONOUS_CONDITION_VARIBLE_CONDITION_VARIBLE_H_*/