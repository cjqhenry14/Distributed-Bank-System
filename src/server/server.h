#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <list>
#include "common.h"

using namespace std;


/*log file*/
extern FILE *out;
extern struct sockaddr_in my_addr;
extern int my_type;
extern int my_port, pre_port, next_port;
extern int sockfd, next_fd, pre_fd;
extern pthread_t acc_next_thread, rec_next_thread, rec_pre_thread, conn_pre_thread, rec_client_thread;
extern pthread_t rec_otbank_thread;
/*master*/
extern int masterfd;
extern pthread_t conn_master_thread, rec_master_thread;
extern pthread_t round_ask_thread;
extern int master_port;
extern char master_ip[16];

extern char my_ip[16], pre_ip[16], next_ip[16];
extern string bank_name;
/*whether connected with previous and next server*/
extern bool connected_pre, connected_next;

extern int server_id, bank_id;
extern int startup_delay;
extern int chain_length;
extern string life_time;
extern int msg_bound;
extern int receive_num;//rec_msg_from_pre_server.FORWORD_REQ, rec_client
extern int send_num;//forward_req_to_next_server, reply_to_client

extern bool is_recovering;//11.8
extern int last_received_id;
extern bool inter_fail_plus, inter_fail_minus;
extern bool extend_new_fail, extend_tail_fail;
extern bool recv_transfer, send_transfer;

extern int req_seq;
/*<account_num, balance> hash table*/
extern unordered_map<int , float> balance_map;
/*processed transitions vector*/
extern vector<request_struct> processed_trans;
/*sent list*/
extern list<request_struct> sent;
/*inSufFundsTrans list*/
extern vector<request_struct> insuf_funds_trans;

extern vector<int> all_heads;
extern vector<int> all_tails;
/*sent list struct*/
struct sent_sn_list_struct
{
    //vector<request_struct> sent_sn_list;
    vector<int> sent_sn_list;
};
/*init server and get necessary info, ip, port. etc.*/
void init_server(int bankid, int serverid);
/*forware request to next server*/
void forward_req_to_next_server(request_struct req);
/*ack to the pre server*/
void ack_to_pre_server(request_struct req);
/*receive message from the next server*/
void *rec_msg_from_next_server(void *);
/*receive message from the previous server*/
void *rec_msg_from_pre_server(void *);
/*accept connect from the next server*/
void *accept_connect_from_next_server(void *);
/*connect to the previous server*/
void *connect_pre_server(void *);
/*init listening to the next server*/
void init_listen_next_server();

void send_next_larger_sn(int sn);
/*check life time*/
void check_my_life();
/*send new server processed_trans, insuf_funds_trans, sent.*/
void send_next_hist_sent();
/*after get larger than sn*/
void after_get_larger_sn(request_struct req);
/*after get request from pre server, main process*/
void process_forwared_request(request_struct req);
/*send transfer req to destBank's head*/
void send_transfer_dest_head(request_struct req);
/*send ack to source bank*/
void ack_to_source_bank(request_struct req);
/*THREAD: receive from other bank: receive transfer req, receive transfer ack*/
void *rec_other_bank(void *args);
/*other bank's head fail, resend the transfer requests in sent(dest_bank_id == fail_bank_id)*/
void transfer_resend_head_fail(transfer_head_fail_struct thfs);

#endif







