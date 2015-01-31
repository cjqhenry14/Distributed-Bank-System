#include "server.h"
#include "server_client.h"
#include "server_master.h"
#include "server_read_conf.h"
#include "process.h"

/*tail internal head tail&head*/
int master_port;
char master_ip[16];

int my_type;

int my_port, pre_port, next_port;
int sockfd, next_fd, pre_fd;

/*master*/
int masterfd;
pthread_t conn_master_thread, rec_master_thread;
pthread_t round_ask_thread;

pthread_t acc_next_thread, rec_next_thread, rec_pre_thread, conn_pre_thread, rec_client_thread;
pthread_t rec_otbank_thread;
char my_ip[16], pre_ip[16], next_ip[16];
string bank_name;

bool connected_pre, connected_next;

int server_id, bank_id;
int startup_delay;
string life_time;
int msg_bound;//get in config
int receive_num;
int send_num;

int chain_length;
bool is_recovering;
int last_received_id;
int req_seq;

bool inter_fail_plus, inter_fail_minus;
bool extend_new_fail, extend_tail_fail;
bool recv_transfer, send_transfer;

unordered_map<int, float> balance_map;
vector<request_struct> processed_trans;
list<request_struct> sent;
vector<request_struct> insuf_funds_trans;

vector<int> all_heads;
vector<int> all_tails;

FILE *out;
/*send new server processed_trans, insuf_funds_trans, sent.*/
void send_next_hist_sent()
{
    if (!connected_next)
    {
        printf("not connected with next server!\n");
        return;
    }
    extend_tail_fail = true;
    check_my_life();

    char msg[MAXDATASIZE];
    /*send processed transitions vector*/
    for (int i = 0; i < processed_trans.size(); ++i)
    {
        msg[0] = SEND_PROCE;
        memcpy(&msg[1], &processed_trans[i], sizeof(processed_trans[i]));
        if (send(next_fd, msg, sizeof(msg), 0) == -1)
            perror("send");
        memset(msg, 0, MAXDATASIZE);
        usleep(20000);//20ms
    }
    printf("send processed over\n");
    fprintf(out, "(%-24s) send processed over\n",  get_time());
    fflush(out);
    /*send inSufFundsTrans vector*/
    for (int i = 0; i < insuf_funds_trans.size(); ++i)
    {
        msg[0] = SEND_INSUF;
        memcpy(&msg[1], &insuf_funds_trans[i], sizeof(insuf_funds_trans[i]));
        if (send(next_fd, msg, sizeof(msg), 0) == -1)
            perror("send");
        memset(msg, 0, MAXDATASIZE);
        usleep(20000);//20ms
    }
    printf("send inSufFundsTrans over\n");
    fprintf(out, "(%-24s) send inSufFundsTrans over\n",  get_time());
    fflush(out);
    /*send sent list*/
    for (list<request_struct>::iterator itor = sent.begin(); itor != sent.end(); itor++)
    {
        request_struct req;
        //copy struct
        req.req_seq = itor->req_seq;
        req.sent_id = itor->sent_id;
        strcpy(req.client_ip, itor->client_ip);
        req.client_port = itor->client_port;
        req.type = itor->type;
        req.req_id = itor->req_id;
        req.account_num = itor->account_num;
        req.amount = itor->amount;
        req.status = itor->status;
        req.dest_bank_id = itor->dest_bank_id;//11.20
        req.dest_account_num = itor->dest_account_num;//11.20
        req.source_bank_id = itor->source_bank_id;

        msg[0] = SEND_SENT;
        memcpy(&msg[1], &req, sizeof(req));

        if (send(next_fd, msg, sizeof(msg), 0) == -1)
            perror("send");

        memset(msg, 0, MAXDATASIZE);
        usleep(20000);//20ms
    }
    printf("send sent over\n");
    fprintf(out, "(%-24s) send sent over\n",  get_time());
    fflush(out);
    /*send unordered_map<int , float> balance_map*/
    unordered_map<int, float>::iterator it = balance_map.begin();
    while (it != balance_map.end())
    {
        balance_map_struct bms;
        bms.account_num = it->first;
        bms.balance = it->second;
        msg[0] = SEND_MAP;
        it++;
        memcpy(&msg[1], &bms, sizeof(bms));

        if (send(next_fd, msg, sizeof(msg), 0) == -1)
            perror("send");

        memset(msg, 0, MAXDATASIZE);
        usleep(20000);//20ms
    }
    printf("send balance_map over\n");
    fprintf(out, "(%-24s) send balance_map over\n",  get_time());
    fflush(out);

    printf("send all over\n");
    fprintf(out, "(%-24s) send all over\n",  get_time());
    fflush(out);
    /*now get connected with next server, and send all, change my type*/

    char msg2[1];
    msg2[0] = SEND_END;
    if (send(next_fd, msg2, sizeof(msg2), 0) == -1)//11.11 means extend abort
    {
        printf("cannot send SEND_END\n");
        perror("send");
    }
    else
    {
        if (my_type == HEAD_TAIL)
            my_type = HEAD;
        else if (my_type == TAIL)
            my_type = INTER;
    }
}
void send_next_larger_sn(int sn)
{
    if (!connected_next)
    {
        printf("not connected with next server!\n");
        return;
    }
    printf("should send next %d with larger than sn:%d\n", next_port, sn);
    inter_fail_minus = true;
    check_my_life();
    int i = 0;
    for (list<request_struct>::iterator itor = sent.begin(); itor != sent.end(); itor++)
    {
        if (itor->req_seq > sn)
        {
            usleep(50000);//50ms
            i++;
            request_struct req;
            //copy struct
            req.req_seq = itor->req_seq;
            req.sent_id = itor->sent_id;
            strcpy(req.client_ip, itor->client_ip);
            req.client_port = itor->client_port;
            req.type = itor->type;
            req.req_id = itor->req_id;
            req.account_num = itor->account_num;
            req.amount = itor->amount;
            req.status = itor->status;
            req.dest_bank_id = itor->dest_bank_id;//11.20
            req.dest_account_num = itor->dest_account_num;//11.20
            req.source_bank_id = itor->source_bank_id;

            char msg[MAXDATASIZE];
            msg[0] = LARGER_SN_LIST; //OPCODE

            memcpy(&msg[1], &req, sizeof(req));

            if (send(next_fd, msg, sizeof(msg), 0) == -1)
                perror("send");
        }
    }
    printf("send next %d with larger than sn:%d, size:%d\n", next_port, sn, i);
    fprintf(out, "(%-24s) send next %d with larger than sn:%d, size:%d\n",  get_time(), next_port, sn, i);
    fflush(out);
}
/*forward request to next server*/
void forward_req_to_next_server(request_struct req)
{
    if (!connected_next)
    {
        printf("not connected with next server!\n");
        return;
    }
    send_num++;
    check_my_life();

    char msg[MAXDATASIZE];
    msg[0] = FORWORD_REQ; //OPCODE

    memcpy(&msg[1], &req, sizeof(req));

    if (send(next_fd, msg, sizeof(msg), 0) == -1)
    {
        perror("send");
    }
}
/*send ack to pre server*/
void ack_to_pre_server(request_struct req)
{
    if (!connected_pre)
    {
        printf("not connected with pre server!\n");
        return;
    }
    fprintf(out, "(%-24s) send ack (%s) to pre server\n",  get_time(), req.req_id.c_str());
    fflush(out);

    char msg[MAXDATASIZE];
    msg[0] = ACK; //OPCODE

    memcpy(&msg[1], &req, sizeof(req));

    if (send(pre_fd, msg, sizeof(msg), 0) == -1)
    {
        perror("send");
    }
}
/*receive msg from next server*/
void *rec_msg_from_next_server(void *)
{
    char buf[MAXDATASIZE];
    int numbytes;
    while (1)
    {
        if ((numbytes = recv(next_fd, buf, MAXDATASIZE, 0)) == -1) {}
        buf[numbytes] = '\0';
        if (strlen(buf) != 0)
        {
            /*receive msg type is ack from next server*/
            if (buf[0] == ACK)
            {
                request_struct req;
                /*change char* to struct*/
                memcpy(&req, &buf[1], sizeof(buf) - sizeof(char));

                fprintf(out, "(%-24s) get ack (%s) from next server\n", get_time(), req.req_id.c_str());
                fflush(out);
                /*delete req from sent*/
                delete_sent(req);

                /*continue ack to pre server*/
                if (my_type != HEAD && my_type != HEAD_TAIL)
                    ack_to_pre_server(req);

                if (my_type == HEAD)
                {
                    if (req.type == "transfer" && bank_id == req.dest_bank_id)
                    {
                        ack_to_source_bank(req);//send ack to source bank's head
                    }
                }
            }
            memset(buf, 0, MAXDATASIZE);
        }
    }/*while*/
}
/*receive msg from pre server*/
void *rec_msg_from_pre_server(void *)
{
    char buf[MAXDATASIZE];
    int numbytes;
    while (1)
    {
        if ((numbytes = recv(pre_fd, buf, MAXDATASIZE, 0)) == -1) {}
        if (numbytes > 0)
        {
            buf[numbytes] = '\0';

            if (buf[0] == LARGER_SN_LIST) //get sent list with id larger than sn
            {
                request_struct req;
                memcpy(&req, &buf[1], sizeof(buf) - sizeof(char));

                after_get_larger_sn(req);//11.10
                memset(buf, 0, MAXDATASIZE);//11.10
            }
            /*receive msg type is request forwarded from pre server*/
            else if (buf[0] == FORWORD_REQ)
            {
                request_struct req;
                memcpy(&req, &buf[1], sizeof(buf) - sizeof(char));
                receive_num++;
                check_my_life();
                process_forwared_request(req);//11.10
                memset(buf, 0, MAXDATASIZE);//11.10
            }//end forward
            /*receive processed transitions vector*/
            else if (buf[0] == SEND_PROCE)
            {
                request_struct req;
                memcpy(&req, &buf[1], sizeof(buf) - sizeof(char));

                processed_trans.push_back(req);
                memset(buf, 0, MAXDATASIZE);//11.10
            }//end SEND_PROCE
            else if (buf[0] == SEND_INSUF)
            {
                request_struct req;
                memcpy(&req, &buf[1], sizeof(buf) - sizeof(char));

                insuf_funds_trans.push_back(req);
                memset(buf, 0, MAXDATASIZE);//11.10
            }//end SEND_INSUF
            else if (buf[0] == SEND_SENT)
            {
                request_struct req;
                memcpy(&req, &buf[1], sizeof(buf) - sizeof(char));

                sent.push_back(req);
                memset(buf, 0, MAXDATASIZE);//11.10
            }//end SEND_SENT
            else if (buf[0] == SEND_MAP)
            {
                balance_map_struct bms;
                memcpy(&bms, &buf[1], sizeof(buf) - sizeof(char));
                balance_map[bms.account_num] = bms.balance;

                memset(buf, 0, MAXDATASIZE);//11.10
            }//end SNED_MAP
            else if (buf[0] == SEND_END)
            {
                printf("get all from pre, i am new tail\n");
                fprintf(out, "(%-24s) get all from pre, i am new tail\n",  get_time());
                fflush(out);
                /*now get connected with pre server, and receive all, change my type*/
                my_type = TAIL;

                master_server_struct mss;
                mss.bank_id = bank_id;
                mss.server_port = my_port;
                strcpy(mss.server_ip, my_ip);
                tell_master_newtail(mss);
                memset(buf, 0, MAXDATASIZE);//11.10
            }//end SEND_END

        }
    }/*while*/
}
/*accept connect from next server*/
void *accept_connect_from_next_server(void *)
{
    struct sockaddr_in their_addr;
    socklen_t sin_size = sizeof(struct sockaddr_in);

    if ((next_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
    {
        perror("accept");
        exit(1);
    }
    //11.10 remove something
    connected_next = true;
    printf("got connection from next server: %d\n", next_port);
    fprintf(out, "(%-24s) got connection from next server: %d\n",  get_time(), next_port);
    fflush(out);
    //11.10 send new server processed_trans, insuf_funds_trans, sent.
    if (my_type == TAIL || my_type == HEAD_TAIL)
        send_next_hist_sent();

    /*create thread to receive next server*/
    if ((pthread_create(&rec_next_thread, NULL, rec_msg_from_next_server, NULL)) != 0)
    {
        printf("create thread error!\r\n");
        exit(1);
    }
    pthread_join(rec_next_thread, NULL);
}
/*connect to pre server*/
void *connect_pre_server(void *)
{
    struct hostent *he;
    /*get pre server's ip*/
    if ((he = gethostbyname(pre_ip)) == NULL)
    {
        perror("gethostbyname");
        exit(1);
    }
    /*create socket*/
    if ((pre_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    /*init sockaddr_in struct*/
    struct sockaddr_in their_addr;
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(pre_port);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    bzero(&(their_addr.sin_zero), 8);
    /*send connect signal to pre server*/
    if (connect(pre_fd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("connect");
        exit(1);
    }
    connected_pre = true;

    printf("got connection from before: %d\n", ntohs(their_addr.sin_port));
    fprintf(out, "(%-24s) got connection from before: %d\n",  get_time(), ntohs(their_addr.sin_port));
    fflush(out);

    /*create thread to receive pre server*/
    if ((pthread_create(&rec_pre_thread, NULL, rec_msg_from_pre_server, NULL)) != 0)
    {
        printf("create thread error!\r\n");
        exit(1);
    }
}

void init_listen_next_server()
{
    /*create socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }
    /*init sockaddr_in*/
    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(my_port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(my_addr.sin_zero), 8);

    /*bind socket and port*/
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }
    /*listen server's socket*/
    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }
    printf("listening...\n");

    /*receive next server connect*/
    if ((pthread_create(&acc_next_thread, NULL, accept_connect_from_next_server, NULL)) != 0)
    {
        printf("create accept thread error!\n");
        exit(1);
    }
}

void init_server(int bankid, int serverid)
{
    if (bankid < 1 || serverid < 1)
    {
        printf("illegal server info input\n");
        return;
    }
    /*ignore broken pipe*/
    signal(SIGPIPE, SIG_IGN);

    is_recovering = false;
    last_received_id = 0;
    req_seq = 0;
    inter_fail_plus = false, inter_fail_minus = false;
    extend_new_fail = false, extend_tail_fail = false;
    recv_transfer = false, send_transfer = false;
    //receive_num:rec_msg_from_pre_server.FORWORD_REQ, rec_client
    //send_num:forward_req_to_next_server, reply_to_client
    receive_num = 0, send_num = 0;
    /*init udp socket to client*/
    init_udp_client();

    bank_id = bankid;
    server_id = serverid;
    connected_pre = false, connected_next = false;
    /*get the current server's information from the config file */
    /* in server_read_conf.cpp */
    get_server_need_info();

    int t = 0;
    while (1) //wait until meet startup delay
    {
        if (startup_delay == t)
            break;
        printf("time: %d\n", t);
        t++;
        sleep(1);
    }
    printf("server start!\n");
    fprintf(out, "(%-24s) server %d start! bank name:%-8s chain length:%-8d\n",
            get_time(), serverid, bank_name.c_str(), chain_length);
    fflush(out);
    /*connect pre server, and get pre server's ip, port, then connect to pre*/
    pthread_create(&conn_master_thread, NULL, connect_master, NULL);

    /*linsten to next server to connect current server*/
    init_listen_next_server();

    pthread_create(&rec_client_thread, NULL, rec_client, NULL);

    pthread_create(&rec_otbank_thread, NULL, rec_other_bank, NULL);
}

int main(int argc, char *argv[])//bank_id  server_id
{
    if (argc != 3)
    {
        fprintf(stderr, "input error\n");
        exit(1);
    }
    string q1 = argv[1], q2 = argv[2];
    string my_log_file = "../../logs/bank" + q1 + "/server-" + q2 + ".txt";
    out = fopen(my_log_file.c_str(), "w+" );

    /*init server and get server's information from the config file */
    init_server(atoi(argv[1]), atoi(argv[2]));

    pthread_join(conn_pre_thread, NULL);
    pthread_join(acc_next_thread, NULL);
    pthread_join(rec_client_thread, NULL);

    pthread_join(conn_master_thread, NULL);
    pthread_join(rec_master_thread, NULL);
    pthread_join(rec_otbank_thread, NULL);

    printf("end\n");

    return 0;
}/*main*/

//========= other function ===============
/*check life time*/
void check_my_life()
{
    if (life_time == "receive")
    {
        if (receive_num == msg_bound)
        {
            printf("die: meet receive message bound:%d\n", msg_bound);
            exit(1);
        }
    }
    else if (life_time == "send")
    {
        if (send_num == msg_bound)
        {
            printf("die: meet send message bound:%d\n", msg_bound);
            exit(1);
        }
    }
    else if (life_time == "remove_inter_fail+")//for remove inter s, and s+ fails when s+ receiv sent from s-.
    {
        if (inter_fail_plus)
        {
            printf("die: meet remove s+ fail condition\n");
            exit(1);
        }
    }
    else if (life_time == "remove_inter_fail-")//remove inter s, s- fails when s- know what to forward to s+.
    {
        if (inter_fail_minus)
        {
            printf("die: meet remove s- fail condition\n");
            exit(1);
        }
    }
    else if (life_time == "extend_newfail")//extend when new abort fail
    {
        if (extend_new_fail)
        {
            printf("die: meet extend abort fail condition\n");
            exit(1);
        }
    }
    else if (life_time == "extend_tailfail")//extend when cur tail fail
    {
        if (extend_tail_fail)
        {
            printf("die: meet extend current tail fail condition\n");
            exit(1);
        }
    }
    else if (life_time == "recv_transfer")//receive transfer then fail
    {
        if (recv_transfer)
        {
            printf("die: meet receive transfer fail condition\n");
            exit(1);
        }
    }
    else if (life_time == "send_transfer")//send transfer then fail
    {
        if (send_transfer)
        {
            printf("die: meet send transfer fail condition\n");
            exit(1);
        }
    }
}
void after_get_larger_sn(request_struct req)
{
    printf("get sent larger than sn from %d, #%d\n", pre_port, req.req_seq);
    fprintf(out, "(%-24s) get sent larger than sn from %d, #%d\n",
            get_time(), pre_port, req.req_seq);
    fflush(out);

    inter_fail_plus = true;
    check_my_life();//for remove inter s, and s+ fails when s+ receiv sent from s-.
    if (!check_in_sent(req))//req.in_sent
    {
        sent.push_back(req);//update my sent

        if (req.status == Need)//if resend, no need to process again
        {
            update_balance(req);
            /*append tail's processed_trans vector*/
            req.status = Processed;
            processed_trans.push_back(req);
            req.status = Need;

            fprintf(out, "(%-24s) APPEND PROCESSED TRANS:  type:%-10s reqID:%-8s accountNum:%-6d\n",
                    get_time(), req.type.c_str(), req.req_id.c_str(), req.account_num);
            fflush(out);
        }
        if (req.status == InsufficientFunds) //append insuf_funds_trans
        {
            insuf_funds_trans.push_back(req);
        }
    }
}

/*after get request from pre server, main process*/
void process_forwared_request(request_struct req)
{
    printf("UPDATE#%d. from pre ser #%d: %s\n", req.req_seq, req.sent_id, req.req_id.c_str());
    last_received_id = req.req_seq;//11.9

    if (req.type == "deposit" || req.type == "withdraw")
    {
        fprintf(out, "(%-24s) UPDATE#%d. RECV #%d:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f\n",
                get_time(), req.req_seq, req.sent_id, req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount);
        fflush(out);
    }
    else if (req.type == "transfer")
    {
        fprintf(out, "(%-24s) UPDATE#%d. RECV #%d:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f destBank:%-3d destAccount:%-6d\n",
                get_time(), req.req_seq, req.sent_id, req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount, req.dest_bank_id, req.dest_account_num);
        fflush(out);
    }
    else
    {
        fprintf(out, "(%-24s) UPDATE#%d. RECV #%d:  type:%-10s reqID:%-8s accountNum:%-6d\n",
                get_time(), req.req_seq, req.sent_id, req.type.c_str(), req.req_id.c_str(), req.account_num);
        fflush(out);
    }
    //----------------
    if (req.status == Need && !check_in_sent(req))//if resend, no need to process again
    {
        update_balance(req);
        /*fprintf(out, "(%-24s) UPDATE BALANCE:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f\n",
                get_time(), req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount);
        fflush(out);*/
        /*append tail's processed_trans vector*/
        req.status = Processed;
        processed_trans.push_back(req);
        req.status = Need;
        fprintf(out, "(%-24s) APPEND PROCESSED TRANS:  type:%-10s reqID:%-8s accountNum:%-6d\n",
                get_time(), req.type.c_str(), req.req_id.c_str(), req.account_num);
        fflush(out);
    }
    if (req.status == InsufficientFunds && !check_in_sent(req)) //append insuf_funds_trans
    {
        insuf_funds_trans.push_back(req);
    }
    //----------------
    if (my_type == INTER)
    {
        /*put current request into sent*/
        if (!check_in_sent(req))
            sent.push_back(req);
        /*forward request*/
        forward_req_to_next_server(req);
    }
    else if (my_type == TAIL)/*processed by tail*/ //11.8
    {
        if (!check_in_sent(req))//11.8 not resend
        {
            sent.push_back(req);//11.8
        }
        else// resend
        {
            req.status = get_pre_send_status(req);//get first send status
        }
        //11.20 if transfer, and i am the source tail, sendto destBank's head
        //no need to reply client
        if (req.type == "transfer" && bank_id != req.dest_bank_id && req.status == Need)
        {
            send_transfer_dest_head(req);//send transfer req to destBank's head
        }
        else if (req.type == "transfer" && bank_id == req.dest_bank_id)
        {
            delete_sent(req);//11.20
            ack_to_pre_server(req);
        }
        else
        {
            struct reply_struct rpy;
            rpy.sent_id = req.sent_id;
            rpy.req_id = req.req_id;
            rpy.account_num = req.account_num;
            rpy.balance = get_account_balance(req.account_num);
            if (req.status == Need)
                rpy.outcome = Processed;
            else
                rpy.outcome = req.status;

            delete_sent(req);//11.8
            /*send ack to pre server*/
            ack_to_pre_server(req);
            /*reply to client*/
            reply_to_client(req, rpy);
        }
    }
}
//******************** FOR TRANSFER *************************
/*THREAD: receive from other bank: receive transfer req, receive transfer ack*/
/*in transfer communication, plus the port with 200*/
void *rec_other_bank(void *args)//now only ack, later: recv sent from s-
{
    int otbankfd;
    char buf[MAXDATASIZE];
    struct sockaddr_in bank_addr;
    bzero(&bank_addr, sizeof(bank_addr));
    bank_addr.sin_family = AF_INET;
    bank_addr.sin_addr.s_addr = inet_addr(my_ip);
    bank_addr.sin_port = htons(my_port + 200);
    socklen_t bank_len = sizeof(bank_addr);
    if ((otbankfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("error opening socket\n");
        exit(1);
    }
    if (bind(otbankfd, (struct sockaddr *)&bank_addr, sizeof(bank_addr)) == -1)
    {
        printf("error in bind\n");
        exit(1);
    }
    while (1)
    {
        if (recvfrom(otbankfd, buf, MAXDATASIZE, 0, (struct sockaddr *)&bank_addr, &bank_len) > 0)
        {
            if (buf[0] == TRANS_ACK)/*get transfer ack from dest head*/
            {
                request_struct req;
                memcpy(&req, &buf[1], sizeof(buf) - sizeof(char));
                printf("get transfer ack (%s) from dest bank %d\n", req.req_id.c_str(), req.source_bank_id);
                fprintf(out, "(%-24s) get transfer ack (%s) from dest bank\n", get_time(), req.req_id.c_str());
                fflush(out);
                delete_sent(req);
                /*reply to client*/
                struct reply_struct rpy;
                rpy.sent_id = req.sent_id;
                rpy.req_id = req.req_id;
                rpy.account_num = req.account_num;
                rpy.balance = get_account_balance(req.account_num);
                if (req.status == Need)
                    rpy.outcome = Processed;
                else
                    rpy.outcome = req.status;

                reply_to_client(req, rpy);
                /*continue ack to pre server*/
                if (my_type != HEAD && my_type != HEAD_TAIL)
                    ack_to_pre_server(req);

                memset(buf, 0, MAXDATASIZE);
            }
        }
    }
}
/*send transfer req to destBank's head*/
/*just regard as client, send function is the same as client*/
void send_transfer_dest_head(request_struct req)
{
    printf("send transfer req %s to dest bank %d\n", req.req_id.c_str(), req.dest_bank_id);
    fprintf(out, "(%-24s) send transfer req %s to dest bank %d\n",  get_time(), req.req_id.c_str(), req.dest_bank_id);
    fflush(out);

    int trans_fd;
    char msg[MAXDATASIZE];

    memcpy(&msg, &req, sizeof(req));

    struct sockaddr_in ser_addr;
    bzero(&ser_addr, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = inet_addr(my_ip);

    int dest_port = all_heads[req.dest_bank_id - 1];
    ser_addr.sin_port = htons(dest_port);

    if ((trans_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Error opening trans_fd socket\n");
    }

    sendto(trans_fd, msg, MAXDATASIZE, 0, (struct sockaddr *)&ser_addr, sizeof(ser_addr));

    if (bank_id == req.source_bank_id)
        send_transfer = true;//11.20
    check_my_life();
}
/*send ack to source bank's tail*/
void ack_to_source_bank(request_struct req)
{
    fprintf(out, "(%-24s) send ack (%s) to source bank\n",  get_time(), req.req_id.c_str());
    fflush(out);

    int ack_fd;
    char msg[MAXDATASIZE];
    msg[0] = TRANS_ACK; //OPCODE

    memcpy(&msg[1], &req, sizeof(req));

    struct sockaddr_in ser_addr;
    bzero(&ser_addr, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = inet_addr(my_ip);

    int source_port = all_tails[req.source_bank_id - 1];//
    ser_addr.sin_port = htons(source_port + 200);

    if ((ack_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Error opening tail_fd socket\n");
        exit(1);
    }

    sendto(ack_fd, msg, MAXDATASIZE, 0, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
}
/*other bank's head fail, resend the transfer requests in sent(dest_bank_id == fail_bank_id)*/
void transfer_resend_head_fail(transfer_head_fail_struct thfs)
{
    int retrans_fd;
    struct sockaddr_in ser_addr;
    bzero(&ser_addr, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = inet_addr(my_ip);

    int dest_port = all_heads[thfs.fail_bank_id - 1];
    ser_addr.sin_port = htons(dest_port);

    if ((retrans_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        printf("Error opening tail_fd socket\n");

    for (list<request_struct>::iterator itor = sent.begin(); itor != sent.end(); itor++)
    {
        if (itor->dest_bank_id == thfs.fail_bank_id && itor->type == "transfer")
        {
            usleep(30000);//30ms
            request_struct req;
            //copy struct
            req.req_seq = itor->req_seq;
            req.sent_id = itor->sent_id;
            strcpy(req.client_ip, itor->client_ip);
            req.client_port = itor->client_port;
            req.type = itor->type;
            req.req_id = itor->req_id;
            req.account_num = itor->account_num;
            req.amount = itor->amount;
            req.status = itor->status;
            req.dest_bank_id = itor->dest_bank_id;//11.20
            req.dest_account_num = itor->dest_account_num;//11.20
            req.source_bank_id = itor->source_bank_id;

            printf("resend transfer req %s to dest bank %d\n", req.req_id.c_str(), req.dest_bank_id);
            fprintf(out, "(%-24s) resend transfer req %s to dest bank %d\n",  get_time(), req.req_id.c_str(), req.dest_bank_id);
            fflush(out);

            char msg[MAXDATASIZE];
            memcpy(&msg, &req, sizeof(req));

            sendto(retrans_fd, msg, MAXDATASIZE, 0, (struct sockaddr *)&ser_addr, sizeof(ser_addr));

            memset(msg, 0, MAXDATASIZE);
        }
    }
}