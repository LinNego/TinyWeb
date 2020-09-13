#ifndef LOG_H_
#define LOG_H_

#include "block_queue.h"
#include <cstdio>
#include <string>
#include <pthread.h>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
using std::string;

class Log {
public:

    static Log* GetInstance();

    /*异步日志*/
    static void *ThreadAsyn(void *args);

    bool Init(const char *file_name, int max_line, bool close_log, bool syn = 1);

    void WriteLog(int level, const char *format, ...);

    void Flush(void);
private:
    void AsynWrite();
    char file_dir_[128];   //服务器要7 * 24，还不止996，难顶
    char file_name_[128];
    int max_lines_;
    long long count_;
    int today_;
    FILE *fp_;
    BlockQueue *block_queue_;
    bool syn_;        //同步标志
    bool close_log_;
    Mutex mutex_;
};

#define LOG_DEBUG(format, ...) if(0 == close_log_) {Log::GetInstance()->WriteLog(0, format, ##__VA_ARGS__); Log::GetInstance()->Flush();}
#define LOG_INFO(format, ...) if(0 == close_log_) {Log::GetInstance()->WriteLog(1, format, ##__VA_ARGS__); Log::GetInstance()->Flush();/*printf("info\n");*/}
#define LOG_WARN(format, ...) if(0 == close_log_) {Log::GetInstance()->WriteLog(2, format, ##__VA_ARGS__); Log::GetInstance()->Flush();}
#define LOG_ERROR(format, ...) if(0 == close_log_) {Log::GetInstance()->WriteLog(3, format, ##__VA_ARGS__); Log::GetInstance()->Flush();}

/*同步日志和异步日志的区别就是同步日志经调用记录日志函数之后，直接写入文件，等到写完才返回*/
/*异步日志则借助缓冲区，调用日志函数，是直接写入缓冲区，内核调度到写线程，写线程从阻塞队列取出日志，然后写入文件*/



#endif