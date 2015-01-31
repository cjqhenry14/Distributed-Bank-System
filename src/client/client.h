#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <queue>
#include "common.h"

using namespace std;
//==============  global ========================

extern int head_port;
extern int tail_port;
extern int my_port;

extern char my_ip[16];
extern char head_ip[16];
extern char tail_ip[16];

extern int master_port;
extern char master_ip[16];

extern int client_id;
extern string bank_name;
extern int bank_id;
extern int wait_time;
extern int resend_times;
extern bool whether_resend_newhead;
extern queue <request_struct> req_que;

extern pthread_t rec_thread;

/*init server and get necessary info, ip, port. etc.*/
void init_client(int bankid, int clientid);
/*receive reply from the tail server, and master's message(set new head or tail)*/
void *recv_msg(void *args);
/*send query to the tail server*/
void send_tail_server(request_struct req);
/*send update to the head server*/
void send_head_server(request_struct req);
/*include send update to head and query to tail*/
void send_request();

/*================= client with master =======================*/

/*tell master client's bank_id, ip, port*/
void send_master();
/*check whether has received reply from server*/
void *check_receive(void *args);


#endif







