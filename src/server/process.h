#ifndef PROCESS_H
#define PROCESS_H

#include "server.h"

#define NOT_YET 1
#define EXACTLY_SAME_PROCESS 2
#define ONLY_SAME_REQID 3
#define EXACTLY_SAME_INSUF 4

/*get account balance*/
float get_account_balance(int account_num);
/*process query*/
int processQuery(request_struct req);
/*process deposit*/
int processDeposit(request_struct req);
/*process withdraw*/
int processWithdraw(request_struct req);
/*process transfer*/
int processTransfer(request_struct req);
/*receive ack and delete the request from sent*/
void delete_sent(request_struct req);
/*update balance*/
void update_balance(request_struct req);
/*check the request's occurence and the details about it*/
int check_same_req(request_struct req);
/*check whether the request is in sent, if is resend, no need to store in sent.*/
bool check_in_sent(request_struct req);

int get_pre_send_status(request_struct req);


#endif