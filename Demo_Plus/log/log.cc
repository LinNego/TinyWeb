#include "log.h"

Log* Log::GetInstance() {
    static Log log;
    return &log;
}

void* Log::ThreadAsyn(void *arg) {
    Log::GetInstance()->AsynWrite();
    return arg;
}


bool Log::Init(const char *file_name, int max_line, bool close_log, bool syn) {
    if(!syn) {
        block_queue_ = new BlockQueue();
        pthread_t write_pthread;
        pthread_create(&write_pthread, nullptr, ThreadAsyn, nullptr);
        pthread_detach(write_pthread);
    }
    syn_ = syn; 
    close_log_ = close_log; 
    time_t t = time(nullptr);
    struct tm tm;
    localtime_r(&t, &tm);
    const char *p = strrchr(file_name, '/');
    char complete_name[256]; //加上了时间的文件路径和文件名
    memset(complete_name, 0, sizeof(complete_name));
    memset(file_dir_, 0, sizeof(file_dir_));
    memset(file_name_, 0, sizeof(file_name));
    max_lines_ = max_line;
    if(!p) {
        snprintf(complete_name, 255, "%4d_%02d_%02d_%s", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, file_name);
    } 
    else {
        snprintf(file_name_, 128, "%4d_%02d_%02d_%s", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, p + 1);
        strncpy(file_dir_, file_name, p - file_name + 1);
        snprintf(complete_name, 255, "%s%s", file_dir_, file_name_);
    }
    today_ = tm.tm_mday;
    fp_ = fopen(complete_name, "a");
    if(fp_ == nullptr) {   // = 又一次。。。
        //printf("fp_error\n");
        return false;
    }
    return true;
}

void Log::Flush() {
    MutexLock mutexlock(mutex_);
    fflush(fp_);
    return ;
}

void Log::WriteLog(int level, const char *format, ...) {
    time_t t = time(nullptr);
    struct tm tm;
    localtime_r(&t, &tm); //重入
    char s[16];
    memset(s, 0, sizeof(s));
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[info]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }

    //cur_lines++;   这个不用加锁吧
    {
        //MutexLock mutexlock(mutex_);
        mutex_.Lock();
        count_++;
        //printf("count %d\n", count_);
        //printf("today %d %d\n", today_, tm.tm_mday);
        if(today_ != tm.tm_mday  || count_ % max_lines_ == 0) {
            char new_log[256]; 
            fflush(fp_); //why
            fclose(fp_);
            char tail[16];
            snprintf(tail, 128, "%4d_%02d_%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            if (today_ != tm.tm_mday) {
                snprintf(new_log, 255, "%s%s%s", file_dir_, tail, file_name_);
                today_ = tm.tm_mday;
                count_ = 0;
            }
            else {
                snprintf(new_log, 255, "%s%s%s.%lld", file_dir_, tail, file_name_, count_ / max_lines_);
            }

            fp_ = fopen(new_log, "a");
        }
        mutex_.Unlock();
    }
    //printf("}\n");

    va_list valst;
    va_start(valst, format);

    char buf[128];
    int n = snprintf(buf, 50, "%4d-%02d-%02d-%02d-%02d-%02d-%s", 
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, s);
    int m = vsnprintf(buf + n, 128 - n, format, valst);
    buf[m + n] = '\n';
    buf[m + n + 1] = 0;
    //printf("%s\n", buf);

    if(!syn_) {
        block_queue_->Push(string(buf));
    }
    else {
        MutexLock mutexlock(mutex_);
        //if(fp_ == nullptr) printf("fr_ errors\n");
        fputs(buf, fp_);
    }
    va_end(valst);
}

void Log::AsynWrite() {
    string temp;
    
    while(block_queue_->Pop(temp)) {
        MutexLock mutexlock(mutex_); 
        fputs(temp.c_str(), fp_);
    }

}





