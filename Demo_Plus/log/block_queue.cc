#include "block_queue.h"

void BlockQueue::Clear() {
    MutexLock mutexlock(mutex_); 
    queue<string> temp;
    block_queue_.swap(temp);
}

void BlockQueue::Push(string str) {
    MutexLock mutexlock(mutex_);
    block_queue_.push(str);
    size_++;
    condvar_.NotifyAll();
}

bool BlockQueue::Pop(string &str) {
    MutexLock mutexlock(mutex_);
    while(size_ <= 0) {
        int ret = condvar_.Wait(mutex_.GetMutexAddr());
        if(ret != 0) {
            return false;
        }
    }
    str = block_queue_.front();
    block_queue_.pop();
    return true;
}

string BlockQueue::Front() {
    MutexLock mutexlock(mutex_);
    while(size_ < 0) {
        int temp = condvar_.Wait(mutex_.GetMutexAddr());
        if(temp != 0) {
            return string("");
        }
    }
    return  block_queue_.front();
}

string BlockQueue::Back() {
    MutexLock mutexlock(mutex_);
    while(size_ <= 0) {
        int temp = condvar_.Wait(mutex_.GetMutexAddr());
        if(temp != 0) {
            return string("");
        }
    }
    return block_queue_.back();
}

int BlockQueue::Size() {
    MutexLock mutexlock(mutex_);
    return block_queue_.size();
}


