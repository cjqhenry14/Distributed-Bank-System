#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <time.h>
#include <signal.h>

using namespace std;

#define CONFIG_FILE "../../config/bank_config.json"

#define BACKLOG 30

#define HEAD 1
#define INTER 2
#define TAIL 3
#define HEAD_TAIL 4

#define MAXDATASIZE 1024
/*transfer*/
#define TRANS_REQ 'r'
#define TRANS_ACK 'a'
#define TRANS_HEAD_FAIL 'y'
#define TRANS_TAIL_FAIL 'z'

#define FORWORD_REQ 'F'
#define ACK 'A'

#define Processed 1
#define InconsistentWithHistory 2
#define InsufficientFunds 3
#define Need 4

#define SER_RPY 'R'
#define SET_HEAD_TAIL 'S'

#define ACK_LAST 1
#define MA_SER_HEAD 2
#define MA_SER_TAIL 3

#define LIVE 1
#define DIE 2

#define HEAD_FAIL 'H'
#define TAIL_FAIL 'T'
#define INTER_PRE_FAIL 'P'
#define INTER_NEXT_FAIL 'N'
#define GET_PRE 'G'
#define NEXT_SN 'E'
#define LARGER_SN_LIST 'L'
/*for send new tail, extend*/
#define SEND_END 'Q'
#define SEND_INSUF 'i'
#define SEND_PROCE 'p'
#define SEND_SENT 'l'
#define NEW_TAIL 't'
#define SEND_MAP 'm'

extern string Outcome[4];

/*get local time*/
char *get_time();
//======================================
/*request struct*/
struct request_struct
{
    int req_seq;
    int sent_id;//sequence id in the sent  11.8
    char client_ip[16];
    int client_port;
    string type; //getBalance, deposit, withdraw, transfer
    string req_id;//bankName.clientNumber.sequenceNumber
    int account_num;
    float amount;
    int status;//Processed, InconsistentWithHistory, InsufficientFunds
    int dest_bank_id;//11.20
    int dest_account_num;//11.20
    int source_bank_id;
};

/*reply struct*/
struct reply_struct
{
    int sent_id;//sequence id in the sent
    string req_id;
    int account_num;
    int outcome;//Processed, InconsistentWithHistory, InsufficientFunds
    float balance;
};

/*client struct, for client send info to master*/
struct client_master_struct
{
    int bank_id;
    int client_port;
    char client_ip[16];
};
/*set head and tail serfer info for client*/
struct set_head_tail
{
    int bank_id;
    int head_port;
    int tail_port;
    char head_ip[16];
    char tail_ip[16];
};
/*server struct, for master send server's info to client and server
e.g. tell client head and tail, tell server next and previous server*/
struct master_server_struct
{
    int bank_id;
    int server_port;
    char server_ip[16];
};
/*communition with sn*/
struct sn_struct
{
    int last_sn;
};
/*send balance map struct*/
struct balance_map_struct
{
    int account_num;
    float balance;
};
/*for transfer: send bank's head fail struct*/
struct transfer_head_fail_struct
{
    int fail_bank_id;
    int new_head_port;
};
/*for transfer: send bank's tail fail struct*/
struct transfer_tail_fail_struct
{
    int fail_bank_id;
    int new_tail_port;
};

#endif