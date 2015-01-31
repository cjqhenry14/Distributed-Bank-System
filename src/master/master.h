#ifndef MASTER_H
#define MASTER_H

#include <iostream>
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
#include <vector>
#include <list>
#include "common.h"

using namespace std;

extern int master_port;
extern char master_ip[16];

/*init master: get necessary info, such as banks' servers' info.*/
void init_master();

/*server sruct, for master to keep server info*/
struct server_struct
{
	int round_flag;//LIVE = 1, DIE = 2;
    int sockfd;
    int bank_id;
    int server_port;
    char server_ip[16];
};
/*client sruct, for master to keep client info*/
struct client_struct
{
    int client_port;
    char client_ip[16];
};
/*request struct*/
struct bank_struct
{
    list<server_struct> server_list;
    list<client_struct> client_list;
};
/*bank vector*/
extern vector<bank_struct> bank_vector;

/*receive from client*/
void *recv_client(void *args);

/*init master socket address struct*/
void init_master_add();
/*send meg to tell client head and tail server's ip, port*/
void send_client(set_head_tail sht, client_master_struct cms);

/*return head server ip, port*/
master_server_struct get_head_server(int bank_id);
/*return tail server ip, port*/
master_server_struct get_tail_server(int bank_id);

void init_listen_server();
void *accept_connect_from_server(void *);
void *rec_msg_from_server(void *);
void show_server_list();

void *round_ask_receive(void *);

void *round_ask_check(void *);

void update_round_flag(int bank_id, int server_port, int round_flag);

void server_fail_notice(int serfd, master_server_struct mss, char fail_type);

void tell_server_pre(int serfd, master_server_struct pre_ser);

/*multicast to client set new head and tail*/
void multicast_client(int bank_id);

void send_client(set_head_tail sht, client_master_struct cms);

void send_sn_to_pre(int serfd, sn_struct sns);
/*head fail, tell other banks' tails*/
void transfer_head_fail(int fail_bank_id, int new_head_port);//fail_bank_id, new_head
/*tail fail, tell other banks' heads*/
void transfer_tail_fail(int fail_bank_id, int new_tail_port);//fail_bank_id, new_tail
#endif







