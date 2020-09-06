#include <cstdio>
class w {
public:
    void print() {
        printf("???");
    }
};
/*不完整的行存入line*/ //这有可能是性能提升的一个地方，也有可能导致bug
/*缓冲区溢出危险，已经假设每一行的长度不会超过1024个字节了*/
EventInput::LineState EventInput::ReadLine() {
    int start = 0, i = 0;
    for(; i < read_cnt_; ++i) {
        if(buf_[i] == '\t') {
            bool flag = IsComPleteLine(i);
            if(flag) {
                if(i + 2 - start > MAXLINE) return OVERFLOW;
                PhaseLine(start, i + 2);
                start = i + 2;
                ++i;
            }
            else if(flag && no_complete_.no_have_t_ == true){
                if(i + 2 - start + no_complete_n_ > MAXLINE) return OVERFLOW;
                PhaseLine(start, i + 2);  //不完整需要考虑line
                start = i + 2;
                ++i;
            }
            else if(!flag) {
                return BADLINE;
            }
        }
        else if(buf_[i] = '\n') {
            if(no_complete_.have_t_ == true) {
                PhaseLine(start, i + 1);
                start = i + 1;
            }
            else {
                memset(line_, 0, sizeof(line_));
                return BADLINE;
            }
        }
    }
    if(start < read_cnt_) {
        no_complete_n_ = read_cnt_ - start;
        if(no_complete_n_ > MAXLINE) return OVERFLOW;
        if(buf_[read_cnt_ - 1] = '\t') {
            no_complete_.no_have_t_ = true;
        }
        else {
            no_complete_.have_t_ = true;
        }
        memcpy(line_, buf_, no_complete_n_);
        return NOCOMPLETE;
    }
    return OVERFLOW; /*这个状态就是说输入缓冲区保存不了这么多溢出了，得告诉程序*/
}