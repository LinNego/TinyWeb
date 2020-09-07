objects = main.o web_server.o paramater.o event_http.o \
				threadpool.o synchronous.o operate_fd.o
main: ${objects} -lpthread
	g++ -o main ${objects} -lpthread

main.o: main.cc
	g++ -c main.cc
web_server.o: web_server.cc web_server.h
	g++ -c web_server.cc
paramater.o: ./paramater/paramater.cc
	g++ -c ./paramater/paramater.cc	
event_http.o: ./threadpool/event_http.cc
	g++ -c ./threadpool/event_http.cc
threadpool.o: ./threadpool/threadpool.cc
	g++ -c ./threadpool/threadpool.cc
synchronous.o: ./threadpool/synchronous.cc
	g++ -c ./threadpool/synchronous.cc
operate_fd.o: ./threadpool/operate_fd.cc
	g++ -c ./threadpool/operate_fd.cc
clean:
	rm ${objects}
