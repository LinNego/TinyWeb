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
    config.parse_arg(argc, argv);

    WebServer server;
    server.Init(config.PORT, user, passwd, databasename, config.LOGWrite, config.OPT_LINGER, 
                    config.TRIGMode, config.sql_num, config.thread_num, config.close_log, config.actor_model);
    
    
    server.LogWrite();


    server.SqlPool();
    
    server.ThreadPools();

    server.TrigMode();
    
    server.EventListen();
    
    server.EventLoop();
    return 0; 
}