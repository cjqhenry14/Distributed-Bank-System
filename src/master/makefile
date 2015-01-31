#g++ -I  ../common/ -o client.o ../common/common.cpp client.cpp master_read_conf.cpp -lpthread
#g++ -I ../common/ -o server.o server.cpp ../common/common.cpp server_read_conf.cpp process.cpp -lpthread
  CFLAGS = -I ../common/

    master: master.o master_read_conf.o common.o
	g++ -o  master  master.o master_read_conf.o common.o -lpthread

    master.o: master.cpp master_read_conf.cpp ../common/common.cpp master_read_conf.h master.h  ../common/common.h
	g++ -c $(CFLAGS) master_read_conf.cpp ../common/common.cpp master.cpp

    master_read_conf.o: master_read_conf.cpp ../common/common.cpp master_read_conf.h master.h ../common/common.h
	g++ -c $(CFLAGS) master_read_conf.cpp ../common/common.cpp

    common.o: ../common/common.cpp ../common/common.h
	g++ -c $(CFLAGS) ../common/common.cpp


    clean:
	rm *.o