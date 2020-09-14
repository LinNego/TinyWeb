#ifndef THREADPOOL_SYNCHRONOUS_SEM_H_
#define THREADPOOL_SYNCHRONOUS_SEM_H_
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <stdexcept>
class Sem {
public:
    Sem(int num = 0);
    ~Sem();
    void Wait();
    void Post();
private:
    sem_t sem_t_; 
};

#endif /*THREADPOOL_SYNCHRONOUS_SEM_H_*/