#ifndef BLOCK_QUEUE_H_
#define BLOCK_QUEUE_H_

#include <string>
#include <queue>
#include "../synchronous.h"
using std::string;
using std::queue;

class BlockQueue {
public:
    BlockQueue() = default;

    void Clear();

    void Push(string str);

    bool Pop(string &str);

    string Front();

    string Back();


    int Size();
private:
    queue<string> block_queue_;
    Mutex mutex_;
    ConditionVarible condvar_;
    int size_;
};

#endif /*BLOCK_QUEUE_H_*/