#ifndef SERVER_CLIENT_H
#define SERVER_CLIENT_H

#include "server.h"



/*init udp socket with client*/
void init_udp_client();
/*receive client's request*/
void *rec_client(void *args);
/*reply to client*/
void reply_to_client(request_struct req, reply_struct rpy);


#endif







