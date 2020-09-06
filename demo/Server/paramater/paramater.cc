/*paramater.cc文件是paramater.h的实现*/
#include "paramater.h"
#include <stdexcept>
#include <cctype>
#include <iostream>
#include <unistd.h>
#include <cstring>

static const int kMaxThreadNum = 1000;
/*:c:p:a:h*/
void Paramater::ParseArgv() {
    int opt;
    char buf[100];
    while((opt = getopt(argc_, argv_, paramater_type_.c_str())) != -1) {
        int num;
        switch (opt) {
        case 't':
            if(JudgeNum(optarg, num) && num <= kMaxThreadNum) {
                thread_num_ = num;
            }
            else {
                throw std::range_error("指定的线程数量参数错误(0 < threadnum < 1001)\n");
            }
            break;
        case 'p':
            if(JudgePort(optarg)) {
                port_str_ = optarg;
            }
            else {
                throw std::range_error("指定的端口号参数错误(1024 < port < 65525)");
            }
            break;
        case 'a':
            if(JudgeAddr(optarg)) {
                addr_str_ = optarg;
            }
            else {
                throw std::range_error("指定的IP地址格式错误");
            }
            break;
        case 'h':
            help_ = true;  /*可能是多余的*/
            PrintHelp();
            break;
        case 'c':
            if(JudgeNum(optarg, num)) {
                max_connect_ = num;
            }
        case '?':
            sprintf(buf, "没有这个-%c选项", optopt);
            throw std::range_error(buf);
            break;
        case ':':
            sprintf(buf, "-%c选项缺乏参数", optopt);
            throw std::range_error(buf);
            break;
        default:
            break;
        }
    }
}

void Paramater::PrintHelp() {
    fprintf(stderr, "用法：./server [选项　参数]...\n");
    fprintf(stderr, "执行一个Web服务器\n");
    fprintf(stderr, "-t     指定最大线程数(默认5)\n");
    fprintf(stderr, "-p     指定监听端口(大于1024小于65535(默认8080))\n");
    fprintf(stderr, "-a     指定要使用的本机IP地址(对于有多张网卡)\n");
    fprintf(stderr, "-c     指定同时支持最大的客户数(默认100)\n");
    fprintf(stderr, "-h     输出本帮助\n");
}

bool Paramater::JudgeChar(std::string::size_type &i, const std::string::size_type dot, 
                            int &ret, const std::string &ip_addr) {
    ret = 0;
    while(i != dot) {
        if(!isdigit(ip_addr[i])) {
            return false;
        }
        ret = ret * 10 + ip_addr[i] - '0';
        ++i;
    } 
    return true;
}

bool Paramater::JudgeAddr(const std::string &ip_addr) {
    std::string::size_type i = 0, dot;
    int ret = 0, dot_number = 0;
    if(ip_addr.size() > 15 || ip_addr.size() < 7) {
        return false;
    }
    while((dot = ip_addr.find_first_of('.', i)) != std::string::npos) {
        ++dot_number;
        if(!JudgeChar(i, dot, ret, ip_addr) || ret > 255) return false;
        ++i;
    }
    if(dot_number != 3) {
        return false;
    }
    if(!JudgeChar(i, ip_addr.size(), ret, ip_addr) || ret > 255) return false;
    return true;
}

bool Paramater::JudgeNum(const std::string &str, int &num) {
    num = 0;
    for(char ch: str) {
        if(!isdigit(ch)) return false;
        num = num * 10 + ch - '0';
    }
    return true;
}

bool Paramater::JudgePort(const std::string &port_num) {
    if(port_num.size() > 5) return false;
    int portnum = 0;
    for(char ch: port_num) {
        if(!isdigit(ch)) return false;
        portnum = portnum * 10 + ch - '0';
    }
    if(portnum > 65535 || portnum < 1024) return false;
    return true;
}