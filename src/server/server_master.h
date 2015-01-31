#ifndef SERVER_MASTER_H
#define SERVER_MASTER_H

#include "server.h"


void *connect_master(void *);

/*receive msg from master*/
void *rec_msg_from_master(void *);

void *round_ask(void *);

void send_to_master(int last_sn);

void listen_next_again();
/*TCP: tell master i am the new tail, for extend*/
void tell_master_newtail(master_server_struct mss);

#endif
