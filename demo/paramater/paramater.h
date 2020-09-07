/*begin parse_paramater.h*/
/*这个头文件是一个包含了需要命令行参数的类*/
/*包含了几个函数的声明*/
#ifndef SERVER_PARAMATER_PARAMATER_H_
#define SERVER_PARAMATER_PARAMATER_H_
#include <string>
#include <stdexcept>

/*作为解析命令行得到的参数结构体*/
/*功能是解析命令行参数之后，判断格式是否正确*/
/*至于有没有效，则在另外一个模块判断*/
class WebServer;
class Paramater {
friend class WebServer;
public:
    Paramater(int argc_, char **argv_)
            : argc_(argc_), argv_(argv_), paramater_type_(":c:p:a:t:h"){ 
                thread_num_ = 5;
                max_connect_ = 100;
                port_str_ = "12345",
                help_ = false;
            }
    void ParseArgv();
    bool help_;             //-h
private:
    int argc_;             //命令行参数个数
    char **argv_;          //命令行参数
    int thread_num_ ;      //-t  最大线程数
    int max_connect_;      // -c 允许的最大客户数
    std::string port_str_;        //-p  端口号
    std::string addr_str_;           //-a
    std::string paramater_type_; //“c:p:a:h"
    /* -i   ipv4 or ipv6    */
    void PrintHelp();
    bool JudgeAddr(const std::string&);
    bool JudgePort(const std::string&);
    bool JudgeNum(const std::string&, int&);
    bool JudgeChar(std::string::size_type&, const std::string::size_type, int&, const std::string&);
};




#endif  //SERVER_PARAMATER_PARAMATER_H_