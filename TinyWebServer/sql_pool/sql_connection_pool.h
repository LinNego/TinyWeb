#ifndef _SQL_CONNECTION_POOL_H_
#define _SQL_CONNECTION_POOL_H_

#include <stdio.h>
#include <queue>
#include <mysql/mysql.h>
#include <error.h>
#include <cstring>
#include "../synchronous.h"
using std::string;

class SqlConnectionPool {
public:
    SqlConnectionPool(const SqlConnectionPool&) = delete; 
    SqlConnectionPool& operator=(const SqlConnectionPool&) = delete;
public:
    static SqlConnectionPool *GetInstance();
    void Init(string&, string&, string&, int, int);
    MYSQL* GetConnection();
    void RealeaseConnection(MYSQL*);
    void FreeConnectionPool();
private:
    SqlConnectionPool();
    ~SqlConnectionPool() { FreeConnectionPool(); }
    /*
    MYSQL * STDCALL  mysql_real_connect(MYSQL *mysql, const char *host,
const char *user,
const char *passwd,
const char *db,
unsigned int port,
const char *unix_socket,
unsigned long clientflag);
)
*/
    int maxconn_, freeconn_;
    string user_;
    string passwd_;
    string database_; 
    int port_;
    Sem sem_;
    Mutex mutex_;
    std::queue<MYSQL*> sql_pool_;
};


/*彻底的RAII*/
class Sql {
public:
    Sql(MYSQL **, SqlConnectionPool*);
    ~Sql();
private:
    MYSQL *sql_;
    SqlConnectionPool *sqlconnectionpool_;
};
#endif /*end of _SQL_CONNECTION_POOL_H_*/