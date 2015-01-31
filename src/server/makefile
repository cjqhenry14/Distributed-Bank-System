#g++ -I  ../common/ -o client.o ../common/common.cpp client.cpp client_read_conf.cpp -lpthread
#g++ -I ../common/ -o server.o server.cpp ../common/common.cpp server_read_conf.cpp process.cpp -lpthread
  CFLAGS = -I ../common/

    server: server.o server_client.o server_master.o server_read_conf.o common.o process.o
	g++ -o  server  server.o server_client.o server_master.o server_read_conf.o common.o process.o -lpthread

    server.o: server.cpp server_client.cpp server_master.cpp server_read_conf.cpp ../common/common.cpp server_read_conf.h server.h server_client.h ../common/common.h
	g++ -c $(CFLAGS) server_read_conf.cpp ../common/common.cpp server.cpp server_client.cpp server_master.cpp

    server_client.o: server_client.cpp server.cpp process.cpp process.h server_client.h server.h ../common/common.h
	g++ -c $(CFLAGS) server_client.cpp server.cpp process.cpp

	server_master.o: server_master.cpp server.cpp server_master.h server.h server_client.h ../common/common.h
	g++ -c $(CFLAGS) server_master.cpp server.cpp server_client.cpp

    process.o: process.cpp  ../common/common.cpp server.h process.h ../common/common.h
	g++ -c $(CFLAGS) process.cpp ../common/common.cpp

    server_read_conf.o: server_read_conf.cpp ../common/common.cpp server_read_conf.h server.h ../common/common.h
	g++ -c $(CFLAGS) server_read_conf.cpp ../common/common.cpp

    common.o: ../common/common.cpp ../common/common.h
	g++ -c $(CFLAGS) ../common/common.cpp


    clean:
	rm *.o