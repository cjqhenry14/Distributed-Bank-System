
INSTRUCTIONS:
Compile and Run code in the unix/linux system.

Compile Server Code: 
1.  cd  bank_chain/src/server/
2.  make

Compile Client Code:
1.  cd  bank_chain/src/client/
2.  make

Compile Master Code: 
1.  cd  bank_chain/src/master/
2.  make

Run Server:
1.  cd  bank_chain/src/server/
2.  ./server bank_id server_id (e.g. for bank 1, start the 2nd server, just ./server 1 2)

Run Client:
1.  cd  bank_chain/src/client/
2.	./client bank_id client_id (e.g. for bank 1, start the 2nd client, just ./client 1 2)

Run Master:
1.  cd  bank_chain/src/master/
2.  ./master

*********** run testcases 1 ***********
1.	open 4 terminals  and  for each terminal:
	./server 1 1	./server 1 2 	./server 2 1 	./server 2 2
2.	open 4 terminals  and  for each terminal:
	run  ./client 1 1    ./client 1 2 	./client 2 1    ./client 2 2
	./client 1 3   ./client 1 4   ./client 1 5   ./client 1 6 


*********** run testcases 2 ***********
1.	open 4 terminals  and  for each terminal:
	./server 3 1	./server 3 2 	./server 4 1 	./server 4 2
2.	open 4 terminals  and  for each terminal:
	run  ./client 3 1    ./client 3 2 	./client 4 1    ./client 4 2

*********** run testcases 3 ***********
1.	open 4 terminals  and  for each terminal:
	./server 5 1	./server 5 2 	./server 6 1 	./server 6 2
2.	open 4 terminals  and  for each terminal:
	run  ./client 5 1    ./client 5 2 	./client 6 1    ./client 6 2

All results will be shown in bank_chain/logs

================================

MAIN FILES:

/*the main part of the server, and include communication between server and server*/
bank_chain/src/server/server.cpp

/*communication between server and client*/
bank_chain/src/server/server_client.cpp

/*communication between server and master*/
bank_chain/src/server/server_master.cpp

/*process request*/
bank_chain/src/server/process.cpp

/*the main part of the client*/
bank_chain/src/client/client.cpp

/*the main part of the master*/
bank_chain/src/master/master.cpp

================================

BUGS AND LIMITATIONS:

If the client sends requests too fast, the server will get several data packages together.
So each client sends request between 0.5s.

================================

LANGUAGE COMPARISON: 
comparison of your experience  implementing the algorithm in the two languages.
I use the C/C++ to implement the project. Compare with DistAlgo, C/C++ has much more code, and DistAlgo is easier to implement. It takes me more time to complete than my teammate. Therefore, DistAlgo is more suitable for distribuited system, due to its fast development speed and good performance. And also, DistAlgo is much more readable than C/C++, because it supports many useful class, function and attribute. C/C++ is flexible, but I don't think it is a good choice to use this language in distributed system, it will cost programmers much more time, and it is much easier to make bugs.  

================================

CONTRIBUTIONS:

Transfer: Jiaqi Chen (C/C++ version)

