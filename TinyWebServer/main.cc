#include <cstdio>
#include <string>
#include "config/config.h"
#include "web_server.h"
using namespace std;

int main(int argc, char *argv[]) {
    string user = "root";
    string passwd = "592106194";
    string databasename = "TinyWebServer";
     
    Config config;
    config.ParseArg(argc, argv);
    if(config.help_) exit(0);

    WebServer server;
    server.Init(config.port_, user, passwd, databasename, config.logwrite_, config.opt_linger_, 
                    config.trigmode_, config.sql_num_, config.thread_num_, config.close_log_, config.actor_model_);
    
    
    server.LogWrite();


    server.SqlPool();
    
    server.ThreadPools();

    server.TrigMode();
    
    server.EventListen();
    
    server.EventLoop();
    return 0; 
}