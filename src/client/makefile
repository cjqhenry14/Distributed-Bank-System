#g++ -I  ../common/ -o client.o ../common/common.cpp client.cpp client_read_conf.cpp -lpthread
#g++ -I ../common/ -o server.o server.cpp ../common/common.cpp server_read_conf.cpp process.cpp -lpthread
  CFLAGS = -I ../common/

    client: client.o client_read_conf.o common.o
	g++ -o  client  client.o client_read_conf.o common.o -lpthread

    client.o: client.cpp client_read_conf.cpp ../common/common.cpp client_read_conf.h client.h  ../common/common.h
	g++ -c $(CFLAGS) client_read_conf.cpp ../common/common.cpp client.cpp

    client_read_conf.o: client_read_conf.cpp ../common/common.cpp client_read_conf.h client.h ../common/common.h
	g++ -c $(CFLAGS) client_read_conf.cpp ../common/common.cpp

    common.o: ../common/common.cpp ../common/common.h
	g++ -c $(CFLAGS) ../common/common.cpp


    clean:
	rm *.o