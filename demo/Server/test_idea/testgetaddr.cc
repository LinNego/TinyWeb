#include "network.h"
#include <string>
#include <iostream>
using namespace std;

void test1() {
    struct addrinfo hint, *result;
    memset(&hint, 0, sizeof(hint)); 
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_flags = AI_PASSIVE | AI_NUMERICHOST;
    const char *host = "5000";
    const char *service = "8000";
    int lfd;
    if(getaddrinfo(host, service, &hint, &result) == 0) {
        printf("???\n");
        if(result == nullptr) printf("What the fuck\n");        
        do {
            lfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
            if(lfd == -1) printf("What the fuck\n");
            int ret = bind(lfd, result->ai_addr, result->ai_addrlen);
            if(ret == 0) {
                sockaddr_in *r = (sockaddr_in*)result->ai_addr;
                printf("%u byebye\n", r->sin_addr.s_addr);
                break;
            }
            result = result->ai_next;

        }while(result != nullptr);
    }
}

void test2() {
   const string &p = "hello";
   cout << p << endl;
}
int main() {
    int i = 0;
    while(i++ < 1) {
        printf("1\n");
    }
    return 0;
}