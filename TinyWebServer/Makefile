object = main.o web_server.o threadpool.o http_event.o \
		 sql_connection_pool.o timer.o log.o block_queue.o synchronous.o config.o
main: ${object}
	g++ -o main ${object} -lpthread -lmysqlclient
main.o: main.cc 
	g++ -c main.cc
web_server.o: web_server.cc web_server.h
	g++ -c web_server.cc
threadpool.o: ./threadpool/threadpool.cc ./threadpool/threadpool.h
	g++ -c ./threadpool/threadpool.cc
http_event.o: ./http/http_event.cc ./http/http_event.h
	g++ -c ./http/http_event.cc
sql_connection_pool.o: ./sql_pool/sql_connection_pool.cc ./sql_pool/sql_connection_pool.h
	g++ -c ./sql_pool/sql_connection_pool.cc
timer.o: ./timer/timer.cc ./timer/timer.h
	g++ -c ./timer/timer.cc
log.o: log/log.cc log/log.h
	g++ -c ./log/log.cc ./log/log.h
block_queue.o: log/block_queue.cc log/block_queue.h
	g++ -c log/block_queue.cc
synchronous.o: synchronous.cc synchronous.h
	g++ -c synchronous.cc
config.o: ./config/config.cc ./config/config.h
	g++ -c ./config/config.cc
clean:
	rm -r *.o


