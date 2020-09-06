#include <queue>
#include "paramater/paramater.h"
#include <iostream>
#include <string>
#include "threadpool/event_http.h"
#include "web_server.h"
using namespace std;


int main(int argc, char *argv[]) {
    Paramater paramater(argc, argv);
    paramater.ParseArgv();
    if(paramater.help_) return 0;
    WebServer webserver(paramater);
    webserver.EventListen();
    webserver.EventLoop();
    return 0;
}