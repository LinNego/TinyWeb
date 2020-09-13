#include "http_event.h"

#include <mysql/mysql.h>
#include <fstream>

//定义http响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

Mutex lock;
std::map<string, string> users;

char *HttpEvent::GetLine() { 
    return read_buf_ + start_line_; 
};

sockaddr_in *HttpEvent::GetAddress() {
    return &address_;
}

void HttpEvent::InitmysqlResult(SqlConnectionPool *connpool) {
    //先从连接池中取一个连接
    MYSQL *mysql = NULL;
    Sql mysqlcon(&mysql, connpool);

    //在user表中检索username，passwd数据，浏览器端输入
    if (mysql_query(mysql, "SELECT username,passwd FROM user")) {
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    }

    //从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);

    //返回结果集中的列数
    int num_fields = mysql_num_fields(result);

    //返回所有字段结构的数组
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    //从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result)) {
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp2;
    }
}

//对文件描述符设置非阻塞
int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void addfd(int epollfd, int fd, bool one_shot, int trig_mode) {
    epoll_event event;
    event.data.fd = fd;

    if (trig_mode == 1)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//从内核时间表删除描述符
void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

//将事件重置为EPOLLONESHOT
void modfd(int epollfd, int fd, int ev, int TRIGMode) {
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int HttpEvent::user_count_ = 0;
int HttpEvent::epollfd_ = -1;

//关闭连接，关闭一个连接，客户总量减一
void HttpEvent::CloseConn(bool real_close) {
    if (real_close && (sockfd_ != -1))
    {
        //printf("close %d\n", sockfd_);
        removefd(epollfd_, sockfd_);
        sockfd_ = -1;
        user_count_--;
    }
}

//初始化连接,外部调用初始化套接字地址
void HttpEvent::Init(int sockfd, const sockaddr_in &addr, char *root, int trig_mode,
                     int close_log, string user, string passwd, string sqlname) {
    sockfd_ = sockfd;
    address_ = addr;

    addfd(epollfd_, sockfd, true, trig_mode);
    user_count_++;

    //当浏览器出现连接重置时，可能是网站根目录出错或http响应格式出错或者访问的文件中内容完全为空
    doc_root_ = root;
    trig_mode_ = trig_mode;
    close_log_ = close_log;

    strcpy(sql_user_, user.c_str());
    strcpy(sql_passwd_, passwd.c_str());
    strcpy(sql_name_, sqlname.c_str());

    Init();
}

//初始化新接受的连接
//check_state_默认为分析请求行状态
void HttpEvent::Init() {
    mysql_ = NULL;
    bytes_to_send_ = 0;
    bytes_have_send_ = 0;
    check_state_ = CHECK_STATE_REQUESTLINE;
    linger_ = false;
    method_ = GET;
    url_ = 0;
    version_ = 0;
    content_length_ = 0;
    host_ = 0;
    start_line_ = 0;
    checked_idx_ = 0;
    read_idx_ = 0;
    write_idx_ = 0;
    cgi_ = 0;
    state_ = 0;
    timer_flag_ = 0;
    improv_ = 0;

    memset(read_buf_, '\0', kREAD_BUFFER_SIZE);
    memset(write_buf_, '\0', kWRITE_BUFFER_SIZE);
    memset(real_file_, '\0', kFILENAME_LEN);
}

//从状态机，用于分析出一行内容
//返回值为行的读取状态，有LINE_OK,LINE_BAD,LINE_OPEN
HttpEvent::LINE_STATUS HttpEvent::ParseLine() {
    char temp;
    for (; checked_idx_ < read_idx_; ++checked_idx_) {
        temp = read_buf_[checked_idx_];
        if (temp == '\r') {
            if ((checked_idx_ + 1) == read_idx_)
                return LINE_OPEN;
            else if (read_buf_[checked_idx_ + 1] == '\n') {
                read_buf_[checked_idx_++] = '\0';
                read_buf_[checked_idx_++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n') {
            if (checked_idx_ > 1 && read_buf_[checked_idx_ - 1] == '\r') {
                read_buf_[checked_idx_ - 1] = '\0';
                read_buf_[checked_idx_++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

//循环读取客户数据，直到无数据可读或对方关闭连接
//非阻塞ET工作模式下，需要一次性将数据读完
bool HttpEvent::ReadOnce() {
    if (read_idx_ >= kREAD_BUFFER_SIZE) {
        return false;
    }
    int bytes_read = 0;

    //LT读取数据
    if (trig_mode_ == 0) {
        bytes_read = recv(sockfd_, read_buf_ + read_idx_, kREAD_BUFFER_SIZE - read_idx_, 0);
        read_idx_ += bytes_read;

        if (bytes_read <= 0) {
            return false;
        }

        return true;
    }
    //ET读数据
    else {
        while (true) {
            bytes_read = recv(sockfd_, read_buf_ + read_idx_, kREAD_BUFFER_SIZE - read_idx_, 0);
            if (bytes_read == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;
            }
            else if (bytes_read == 0) {
                return false;
            }
            read_idx_ += bytes_read;
        }
        return true;
    }
}

//解析http请求行，获得请求方法，目标url及http版本号
HttpEvent::HTTP_CODE HttpEvent::ParseRequestLine(char *text) {
    url_ = strpbrk(text, " \t");
    if (!url_) {
        return BAD_REQUEST;
    }
    *url_++ = '\0';
    char *method = text;
    if (strcasecmp(method, "GET") == 0)
        method_ = GET;
    else if (strcasecmp(method, "POST") == 0) {
        method_ = POST;
        cgi_ = 1;
    }
    else
        return BAD_REQUEST;
    url_ += strspn(url_, " \t");
    version_ = strpbrk(url_, " \t");
    if (!version_)
        return BAD_REQUEST;
    *version_++ = '\0';
    version_ += strspn(version_, " \t");
    if (strcasecmp(version_, "HTTP/1.1") != 0)
        return BAD_REQUEST;
    if (strncasecmp(url_, "http://", 7) == 0) {
        url_ += 7;
        url_ = strchr(url_, '/');
    }

    if (strncasecmp(url_, "https://", 8) == 0) {
        url_ += 8;
        url_ = strchr(url_, '/');
    }

    if (!url_ || url_[0] != '/')
        return BAD_REQUEST;
    //当url为/时，显示判断界面
    if (strlen(url_) == 1)
        strcat(url_, "judge.html");
    check_state_ = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//解析http请求的一个头部信息
HttpEvent::HTTP_CODE HttpEvent::ParseHeaders(char *text) {
    if (text[0] == '\0') {
        if (content_length_ != 0) {
            check_state_ = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) {
            linger_ = true;
        }
    }
    else if (strncasecmp(text, "Content-length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        content_length_ = atol(text);
    }
    else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        host_ = text;
    }
    else {
        LOG_INFO("oop!unknow header: %s", text);
    }
    return NO_REQUEST;
}

//判断http请求是否被完整读入
HttpEvent::HTTP_CODE HttpEvent::ParseContent(char *text) {
    if (read_idx_ >= (content_length_ + checked_idx_)) {
        text[content_length_] = '\0';
        //POST请求中最后为输入的用户名和密码
        string_ = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

HttpEvent::HTTP_CODE HttpEvent::ProcessRead() {
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;

    while ((check_state_ == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = ParseLine()) == LINE_OK)) {
        text = GetLine();
        start_line_ = checked_idx_;
        LOG_INFO("%s", text);
        switch (check_state_)
        {
        case CHECK_STATE_REQUESTLINE:
        {
            //printf("Check_ requestline\n");
            ret = ParseRequestLine(text);
            if (ret == BAD_REQUEST)
                return BAD_REQUEST;
            break;
        }
        case CHECK_STATE_HEADER:
        {
            //printf("check_state_header\n");
            ret = ParseHeaders(text);
            if (ret == BAD_REQUEST) {
                //printf("Bad_request\n");
                return BAD_REQUEST;
            }
            else if (ret == GET_REQUEST)
            {
                //printf("GET request\n");
                return DoRequest();
            }
            break;
        }
        case CHECK_STATE_CONTENT:
        {
            //printf("check_state_content\n");
            ret = ParseContent(text);
            if (ret == GET_REQUEST)
                return DoRequest();
            line_status = LINE_OPEN;
            break;
        }
        default:
            //printf("interal_error\n");
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

HttpEvent::HTTP_CODE HttpEvent::DoRequest() {
    strcpy(real_file_, doc_root_);
    int len = strlen(doc_root_);
    //printf("url_:%s\n", url_);
    const char *p = strrchr(url_, '/');

    //处理cgi_
    if (cgi_ == 1 && (*(p + 1) == '2' || *(p + 1) == '3')) {

        //根据标志判断是登录检测还是注册检测
        char flag = url_[1];

        char *url__real = (char *)malloc(sizeof(char) * 200);
        strcpy(url__real, "/");
        strcat(url__real, url_ + 2);
        strncpy(real_file_ + len, url__real, kFILENAME_LEN - len - 1);
        free(url__real);

        //将用户名和密码提取出来
        //user=123&passwd=123
        char name[100], password[100];
        int i;
        for (i = 5; string_[i] != '&'; ++i)
            name[i - 5] = string_[i];
        name[i - 5] = '\0';

        int j = 0;
        for (i = i + 10; string_[i] != '\0'; ++i, ++j)
            password[j] = string_[i];
        password[j] = '\0';

        if (*(p + 1) == '3') {
            //如果是注册，先检测数据库中是否有重名的
            //没有重名的，进行增加数据
            char *sql_insert = (char *)malloc(sizeof(char) * 200);
            strcpy(sql_insert, "INSERT INTO user(username, passwd) VALUES(");
            strcat(sql_insert, "'");
            strcat(sql_insert, name);
            strcat(sql_insert, "', '");
            strcat(sql_insert, password);
            strcat(sql_insert, "')");

            if (users.find(name) == users.end()) {
                lock.Lock();
                int res = mysql_query(mysql_, sql_insert);
                users.insert(pair<string, string>(name, password));
                lock.Unlock();

                if (!res)
                    strcpy(url_, "/log.html");
                else
                    strcpy(url_, "/registerError.html");
            }
            else
                strcpy(url_, "/registerError.html");
        }
        //如果是登录，直接判断
        //若浏览器端输入的用户名和密码在表中可以查找到，返回1，否则返回0
        else if (*(p + 1) == '2') {
            if (users.find(name) != users.end() && users[name] == password)
                strcpy(url_, "/welcome.html");
            else
                strcpy(url_, "/logError.html");
        }
    }

    if (*(p + 1) == '0') {
        char *url__real = (char *)malloc(sizeof(char) * 200);
        strcpy(url__real, "/register.html");
        strncpy(real_file_ + len, url__real, strlen(url__real));

        free(url__real);
    }
    else if (*(p + 1) == '1') {
        char *url__real = (char *)malloc(sizeof(char) * 200);
        strcpy(url__real, "/log.html");
        strncpy(real_file_ + len, url__real, strlen(url__real));

        free(url__real);
    }
    else if (*(p + 1) == '5') {
        char *url__real = (char *)malloc(sizeof(char) * 200);
        strcpy(url__real, "/picture.html");
        strncpy(real_file_ + len, url__real, strlen(url__real));

        free(url__real);
    }
    else if (*(p + 1) == '6') {
        char *url__real = (char *)malloc(sizeof(char) * 200);
        strcpy(url__real, "/video.html");
        strncpy(real_file_ + len, url__real, strlen(url__real));

        free(url__real);
    }
    /*
    else if (*(p + 1) == '7') {
        char *url__real = (char *)malloc(sizeof(char) * 200);
        strcpy(url__real, "/fans.html");
        strncpy(real_file_ + len, url__real, strlen(url__real));

        free(url__real);
    }
    */
    else {
        //printf("默认\n");
        strncpy(real_file_ + len, url_, kFILENAME_LEN - len - 1);
        //printf("real %s\n", real_file_);
    }

    if (stat(real_file_, &file_stat_) < 0) {
        //printf("stat error\n");
        return NO_RESOURCE;
    }

    if (!(file_stat_.st_mode & S_IROTH)) {
        //printf("没有权限\n");
        return FORBIDDEN_REQUEST;
    }

    if (S_ISDIR(file_stat_.st_mode)) {
       // printf("isdir 错误\n");
        return BAD_REQUEST;
    }

    int fd = open(real_file_, O_RDONLY);
    file_address_ = (char *)mmap(0, file_stat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}
void HttpEvent::Unmap() {
    if (file_address_) {
        munmap(file_address_, file_stat_.st_size);
        file_address_ = 0;
    }
}
bool HttpEvent::Write() {
    int temp = 0;

    if (bytes_to_send_ == 0) {
        modfd(epollfd_, sockfd_, EPOLLIN, trig_mode_);
        Init();
        return true;
    }

    while (1) {
        temp = writev(sockfd_, iv_, iv_count_);

        if (temp < 0) {
            if (errno == EAGAIN)
            {
                modfd(epollfd_, sockfd_, EPOLLOUT, trig_mode_);
                return true;
            }
            Unmap();
            return false;
        }

        bytes_have_send_ += temp;
        bytes_to_send_ -= temp;
        if (bytes_have_send_ >= iv_[0].iov_len) {
            iv_[0].iov_len = 0;
            iv_[1].iov_base = file_address_ + (bytes_have_send_ - write_idx_);
            iv_[1].iov_len = bytes_to_send_;
        }
        else {
            iv_[0].iov_base = write_buf_ + bytes_have_send_;
            iv_[0].iov_len = iv_[0].iov_len - bytes_have_send_;
        }

        if (bytes_to_send_ <= 0) {
            Unmap();
            modfd(epollfd_, sockfd_, EPOLLIN, trig_mode_);

            if (linger_) {
                Init();
                return true;
            }
            else {
                return false;
            }
        }
    }
}
bool HttpEvent::AddResponse(const char *format, ...) {
    if (write_idx_ >= kWRITE_BUFFER_SIZE)
        return false;
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(write_buf_ + write_idx_, kWRITE_BUFFER_SIZE - 1 - write_idx_, format, arg_list);
    if (len >= (kWRITE_BUFFER_SIZE - 1 - write_idx_)) {
        va_end(arg_list);
        return false;
    }
    write_idx_ += len;
    va_end(arg_list);

    LOG_INFO("request:%s", write_buf_);

    return true;
}
bool HttpEvent::AddStatusLine(int status, const char *title) {
    return AddResponse("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool HttpEvent::AddHeaders(int content_len) {
    return AddContentLength(content_len) && AddLinger() &&
           AddBlankLine();
}

bool HttpEvent::AddContentLength(int content_len) {
    return AddResponse("Content-Length:%d\r\n", content_len);
}

bool HttpEvent::AddContentType() {
    return AddResponse("Content-Type:%s\r\n", "text/html");
}

bool HttpEvent::AddLinger() {
    return AddResponse("Connection:%s\r\n", (linger_ == true) ? "keep-alive" : "close");
}

bool HttpEvent::AddBlankLine() {
    return AddResponse("%s", "\r\n");
}

bool HttpEvent::AddContent(const char *content) {
    return AddResponse("%s", content);
}

bool HttpEvent::ProcessWrite(HTTP_CODE ret) {
    //printf("进入读取\n");
    switch (ret)
    {
    case INTERNAL_ERROR:
    {
        //printf("InterError\n");
        AddStatusLine(500, error_500_title);
        AddHeaders(strlen(error_500_form));
        if (!AddContent(error_500_form))
            return false;
        break;
    }
    case BAD_REQUEST:
    {
        //printf("Bad_Request\n");
        AddStatusLine(404, error_404_title);
        AddHeaders(strlen(error_404_form));
        if (!AddContent(error_404_form))
            return false;
        break;
    }
    case FORBIDDEN_REQUEST:
    {
        //printf("Forbidden_request\n");
        AddStatusLine(403, error_403_title);
        AddHeaders(strlen(error_403_form));
        if (!AddContent(error_403_form))
            return false;
        break;
    }
    case FILE_REQUEST:
    {
        //printf("File_request\n");
        AddStatusLine(200, ok_200_title);
        if (file_stat_.st_size != 0)
        {
            AddHeaders(file_stat_.st_size);
            iv_[0].iov_base = write_buf_;
            iv_[0].iov_len = write_idx_;
            iv_[1].iov_base = file_address_;
            iv_[1].iov_len = file_stat_.st_size;
            iv_count_ = 2;
            bytes_to_send_ = write_idx_ + file_stat_.st_size;
            return true;
        }
        else
        {
            const char *ok_string = "<html><body></body></html>";
            AddHeaders(strlen(ok_string));
            if (!AddContent(ok_string))
                return false;
        }
    }
    default:
        //printf("????\n");
        return false;
    }
    iv_[0].iov_base = write_buf_;
    iv_[0].iov_len = write_idx_;
    iv_count_ = 1;
    bytes_to_send_ = write_idx_;
    return true;
}

void HttpEvent::Process() {
    HTTP_CODE read_ret = ProcessRead();
    if (read_ret == NO_REQUEST) {
       // printf("NO request\n");
        modfd(epollfd_, sockfd_, EPOLLIN, trig_mode_);
        return;
    }
    //printf("???\n");
    bool Write_ret = ProcessWrite(read_ret);
    if (!Write_ret) {
        CloseConn();
    }
    //printf("不是吧\n");
    modfd(epollfd_, sockfd_, EPOLLOUT, trig_mode_);
}
