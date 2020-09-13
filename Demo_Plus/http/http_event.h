#ifndef HTTP_EVENT_H_
#define HTTP_EVENT_H_

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cassert>
#include <sys/stat.h>
#include <cstring>
#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>
#include <cstdarg>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>
#include <string>
#include <utility>
using std::map;
using std::string;
using std::pair;

#include "../synchronous.h"
#include "../sql_pool/sql_connection_pool.h"
#include "../log/log.h"

static const int kFILENAME_LEN = 200;
static const int kREAD_BUFFER_SIZE = 2048;
static const int kWRITE_BUFFER_SIZE = 1024;

class HttpEvent {
public:
    enum METHOD {
        GET,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };
    int state_;
public:
    HttpEvent() = default;

    void Init(int sockfd, const sockaddr_in &addr, char *, int, int, string user, string passwd, string sqlname);
    void CloseConn(bool real_close = true);
    void Process();
    bool ReadOnce();
    bool Write();
    sockaddr_in *GetAddress();
    void InitmysqlResult(SqlConnectionPool*);
    int timer_flag_;
    int improv_;
private:
     void Init();
    HTTP_CODE ProcessRead();
    bool ProcessWrite(HTTP_CODE ret);
    HTTP_CODE ParseRequestLine(char *text);
    HTTP_CODE ParseHeaders(char *text);
    HTTP_CODE ParseContent(char *text);
    HTTP_CODE DoRequest();
    char *GetLine();
    LINE_STATUS ParseLine();
    void Unmap();
    bool AddResponse(const char *format, ...);
    bool AddContent(const char *content);
    bool AddStatusLine(int status, const char *title);
    bool AddHeaders(int content_length);
    bool AddContentType();
    bool AddContentLength(int content_length);
    bool AddLinger();
    bool AddBlankLine();

public:
    static int epollfd_;
    static int user_count_;
    MYSQL *mysql_;
    int states_;  //读为0, 写为1

private:
    int sockfd_;
    sockaddr_in address_;
    char read_buf_[kREAD_BUFFER_SIZE];
    int read_idx_;
    int checked_idx_;
    int start_line_;
    char write_buf_[kWRITE_BUFFER_SIZE];
    int write_idx_;
    CHECK_STATE check_state_;
    METHOD method_;
    char real_file_[kFILENAME_LEN];
    char *url_;
    char *version_;
    char *host_;
    int content_length_;
    bool linger_;
    char *file_address_;
    struct stat file_stat_;
    struct iovec iv_[2];
    int iv_count_;
    int cgi_;        //是否启用的POST
    char *string_; //存储请求头数据
    int bytes_to_send_;
    int bytes_have_send_;
    char *doc_root_;

    std::map<string, string> users_;
    int trig_mode_;
    int close_log_;

    char sql_user_[100];
    char sql_passwd_[100];
    char sql_name_[100];
};

#endif /*end of HTTP_EVENT_H_*/