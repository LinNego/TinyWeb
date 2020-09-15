#include "config.h"

Config::Config(){
    //端口号,默认9006
    port_ = 9006;

    //日志写入方式，默认同步
    logwrite_ = 0;

    //触发组合模式,默认listenfd LT + connfd LT
    trigmode_ = 0;

    //listenfd触发模式，默认LT
    listentrigmode_ = 0;

    //connfd触发模式，默认LT
    conntrigmode_ = 0;

    //优雅关闭链接，默认不使用
    opt_linger_ = 0;

    //数据库连接池数量,默认8
    sql_num_ = 8;

    //线程池内的线程数量,默认8
    thread_num_ = 8;

    //关闭日志,默认不关闭
    close_log_ = 0;

    //并发模型,默认是proactor
    actor_model_ = 0;
}

void Config::ParseArg(int argc, char*argv[]){
    int opt;
    const char *str = "p:l:m:o:s:t:c:a:h:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt) {

        case 'p':
            port_ = atoi(optarg);
            break;

        case 'l':
            logwrite_ = atoi(optarg);
            break;

        case 'm':
            trigmode_ = atoi(optarg);
            break;

        case 'o':
            opt_linger_ = atoi(optarg);
            break;

        case 's':
            sql_num_ = atoi(optarg);
            break;

        case 't':
            thread_num_ = atoi(optarg);
            break;

        case 'c':
            close_log_ = atoi(optarg);
            break;

        case 'a':
            actor_model_ = atoi(optarg);
            break;
        
        case 'h':
            help_ = true;
            PrintHelp();
            break;
        
        case ':':
            printf("没有这个%c选项\n", optopt);
            exit(1);
            break;

        case '?':
            printf("-%c选项缺乏参数\n", optopt);
            exit(1);
            break;

        default:
            break;
        }
    }
}

void Config::PrintHelp() {
    printf("usage: ./main [选项　参数]... \n");
    printf("执行一个Web服务器\n");
    printf("-p 指定端口号(默认9006)\n");
    printf("-l 指定日志的写入方式(默认同步): 0为同步　1为异步\n");
    printf("-m 指定监听模式(默认水平触发)\n");
    printf("-o 指定是否优雅关闭连接(默认不使用) 1为使用\n");
    printf("-s 指定数据库连接池的数量\n");
    printf("-t 指定线程池线程数量(默认为8)\n");
    printf("-c 指定是否关闭日志(默认开启): 1为关闭　0为开启\n");
    printf("-a 指定并发模型(默认为proactor)　0:为proactor 1为reactor\n");
    printf("-h 输出本帮助\n");
}