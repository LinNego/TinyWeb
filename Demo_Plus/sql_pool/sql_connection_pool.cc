#include "sql_connection_pool.h"

SqlConnectionPool::SqlConnectionPool(): maxconn_(0), freeconn_(0) { }

void SqlConnectionPool::Init(string &user, string &passwd, string &database, int port, int maxconn) { 
    user_ = user;
    passwd_ = passwd;
    database_ = database;
    port_ = port;
    maxconn_ = maxconn;
    for(int i = 0; i < maxconn_; ++i) {
        MYSQL *conn = nullptr;
        conn = mysql_init(conn);
        if(!conn) {
            printf("Mysqls error\n");
            exit(1);
        }
        conn =  mysql_real_connect(conn, "127.0.0.1", user_.c_str(), passwd_.c_str(), database_.c_str(), port_, nullptr, 0);
        if(!conn) {
            printf("mysql error\n");
            exit(1);
        }
        sql_pool_.push(conn);
        freeconn_++;
    }
    maxconn_ = freeconn_;
    sem_ = Sem(maxconn_);
}

MYSQL* SqlConnectionPool::GetConnection() {
    MYSQL *conn;
    sem_.Wait();
    {
        MutexLock mutexlock(mutex_);
        conn = sql_pool_.front();
        sql_pool_.pop();
        --freeconn_;
    }
    return conn;
}

void SqlConnectionPool::RealeaseConnection(MYSQL *conn) {
    if(!conn)  return ;
    {
        MutexLock mutexlock(mutex_);
        sql_pool_.push(conn);
        ++freeconn_;
    }
    sem_.Post();
}

void SqlConnectionPool::FreeConnectionPool() {
    MutexLock mutexlock(mutex_);
    while(!sql_pool_.empty()) {
        MYSQL *temp = sql_pool_.front();
        mysql_close(temp);
        sql_pool_.pop();
    }
}


SqlConnectionPool*
SqlConnectionPool::GetInstance() {
    static SqlConnectionPool instance;
    return &instance;
}

Sql::Sql(MYSQL **sql, SqlConnectionPool *sqlconnectionpool) {
    sqlconnectionpool_ = sqlconnectionpool;
    *sql = sqlconnectionpool_->GetConnection();
    sql_ = *sql;
}

Sql::~Sql() {
    sqlconnectionpool_->RealeaseConnection(sql_);
    sql_ = nullptr;
}


