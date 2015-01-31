#include "server.h"
#include "server_client.h"
#include "process.h"
//=================   about msg with client   ========================
int reply_client_fd;

struct sockaddr_in client_addr;

/*THREAD: receive requests from client*/
void *rec_client(void *args)
{
    int client_fd;
    char buf[MAXDATASIZE];
    struct sockaddr_in cli_addr;
    bzero(&cli_addr, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = inet_addr(my_ip);//my ip
    cli_addr.sin_port = htons(my_port);
    socklen_t cli_len = sizeof(cli_addr);

    if ((client_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        fprintf(out, "(%-24s) error opening socket\n", get_time());
        fflush(out);
        exit(1);
    }
    if (bind(client_fd, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) == -1)
    {
        fprintf(out, "(%-24s) error in bind\n", get_time());
        fflush(out);
        exit(1);
    }

    while (1)
    {
        if (recvfrom(client_fd, buf, MAXDATASIZE, 0, (struct sockaddr *)&cli_addr, &cli_len) > 0)
        {
            receive_num++;
            check_my_life();
            /*receive the request struct*/
            request_struct req;
            /*change char* to struct*/
            memcpy(&req, buf, sizeof(buf));
            memset(buf, 0, MAXDATASIZE);
            //----------------
            if (req.type == "deposit")
            {
                if (check_in_sent(req))//resend
                {
                    printf("RESEND#%d. RECV #%d: %s\n", req.req_seq, req.sent_id, req.req_id.c_str());
                    fprintf(out, "(%-24s) RESEND#%d. RECV #%d:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f\n",
                            get_time(), req.req_seq, req.sent_id, req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount);
                    fflush(out);
                    req.status = get_pre_send_status(req);//get first send status
                }
                else//not resend
                {
                    req_seq++;
                    req.req_seq = req_seq;
                    printf("UPDATE#%d. RECV #%d: %s\n", req.req_seq, req.sent_id, req.req_id.c_str());
                    fprintf(out, "(%-24s) UPDATE#%d. RECV #%d:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f\n",
                            get_time(), req.req_seq, req.sent_id, req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount);
                    fflush(out);

                    req.status = processDeposit(req);//has not been processed and suitable,status=“need”
                }
            }
            else if (req.type == "withdraw")
            {
                if (check_in_sent(req))//resend
                {
                    printf("RESEND#%d. RECV #%d: %s\n", req.req_seq, req.sent_id, req.req_id.c_str());
                    fprintf(out, "(%-24s) RESEND#%d. RECV #%d:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f\n",
                            get_time(), req.req_seq, req.sent_id, req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount);
                    fflush(out);
                    req.status = get_pre_send_status(req);//get first send status
                }
                else//not resend
                {
                    req_seq++;
                    req.req_seq = req_seq;
                    printf("UPDATE#%d. RECV #%d: %s\n", req.req_seq, req.sent_id, req.req_id.c_str());
                    fprintf(out, "(%-24s) UPDATE#%d. RECV #%d:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f\n",
                            get_time(), req.req_seq, req.sent_id, req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount);
                    fflush(out);

                    req.status = processWithdraw(req);//has not been processed and suitable,status=“need”
                }
                if (req.status == InsufficientFunds) //append insuf_funds_trans
                {
                    insuf_funds_trans.push_back(req);
                }
            }
            //11.20
            else if (req.type == "transfer")
            {
                if (check_in_sent(req))//resend
                {
                    printf("TRANS RESEND#%d. RECV #%d: %s\n", req.req_seq, req.sent_id, req.req_id.c_str());
                    fprintf(out, "(%-24s) RESEND#%d. RECV #%d:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f destBank:%-3d destAccount:%-6d\n",
                            get_time(), req.req_seq, req.sent_id, req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount, req.dest_bank_id, req.dest_account_num);
                    fflush(out);

                    if (bank_id == req.dest_bank_id)
                        recv_transfer = true;//11.20
                    check_my_life();

                    req.status = get_pre_send_status(req);//get first send status
                }
                else//not resend
                {
                    req_seq++;
                    req.req_seq = req_seq;
                    printf("TRANS#%d. RECV #%d: %s\n", req.req_seq, req.sent_id, req.req_id.c_str());
                    fprintf(out, "(%-24s) UPDATE#%d. RECV #%d:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f destBank:%-3d destAccount:%-6d\n",
                            get_time(), req.req_seq, req.sent_id, req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount, req.dest_bank_id, req.dest_account_num);
                    fflush(out);

                    if (bank_id == req.dest_bank_id)
                        recv_transfer = true;//11.20
                    check_my_life();

                    req.status = processTransfer(req);
                }
                if (req.status == InsufficientFunds) //append insuf_funds_trans
                {
                    insuf_funds_trans.push_back(req);
                }
            }
            //else if (req.type == "getBalance" && (my_type == TAIL || my_type == HEAD_TAIL))
            else if (req.type == "getBalance" && (my_type == TAIL || my_type == HEAD_TAIL))
            {
                printf("RECV #%d: %s\n", req.sent_id, req.req_id.c_str());
                fprintf(out, "(%-24s) RECV #%d:  type:%-10s reqID:%-8s accountNum:%-6d\n",
                        get_time(), req.sent_id, req.type.c_str(), req.req_id.c_str(), req.account_num);
                fflush(out);

                if (check_in_sent(req))//resend
                    req.status = get_pre_send_status(req);//get first send status
                else//not resend
                    req.status = processQuery(req);
            }
            else
            {
                printf("unknown or wrong request type\n");
                continue;
            }
            //----------------
            if (my_type == HEAD)//update
            {
                /*check whether the request is in sent,
                if is resend, no need to store in sent.*/
                if (!check_in_sent(req))//11.8 not resend
                {
                    sent.push_back(req);/*put current request into sent*/
                    /*append processed_trans vector*/
                    if (req.status == Need)
                    {
                        req.status = Processed;
                        processed_trans.push_back(req);
                        req.status = Need;

                        fprintf(out, "(%-24s) APPEND PROCESSED TRANS:  type:%-10s reqID:%-8s accountNum:%-6d\n",
                                get_time(), req.type.c_str(), req.req_id.c_str(), req.account_num);
                        fflush(out);
                    }
                }
                /*forward request*/
                forward_req_to_next_server(req);
            }
            else if (my_type == TAIL)//query
            {
                struct reply_struct rpy;
                rpy.sent_id = req.sent_id;//11.8
                rpy.req_id = req.req_id;
                rpy.outcome = req.status;
                rpy.account_num = req.account_num;
                rpy.balance = get_account_balance(req.account_num);
                reply_to_client(req, rpy);
            }
            else if (my_type == HEAD_TAIL)//only one server
            {
                if (!check_in_sent(req))//11.8 not resend
                {
                    sent.push_back(req);
                    /*append processed_trans vector*/
                    if (req.status == Need)
                    {
                        req.status = Processed;
                        processed_trans.push_back(req);
                        req.status = Need;//11.20

                        fprintf(out, "(%-24s) APPEND PROCESSED TRANS:  type:%-10s reqID:%-8s accountNum:%-6d\n",
                                get_time(), req.type.c_str(), req.req_id.c_str(), req.account_num);
                        fflush(out);
                    }
                }
                //11.20 if transfer, and i am the source tail, sendto destBank's head
                //no need to reply client
                //or has been processed, don't transfer to dest bank
                if (req.type == "transfer" && bank_id != req.dest_bank_id && req.status == Need)
                {
                    send_transfer_dest_head(req);//send transfer req to destBank's head
                }
                else
                {
                    struct reply_struct rpy;
                    rpy.sent_id = req.sent_id;//11.8
                    rpy.req_id = req.req_id;
                    rpy.account_num = req.account_num;
                    if (req.status == Need)//11.20
                        rpy.outcome = Processed;
                    else
                        rpy.outcome = req.status;

                    rpy.balance = get_account_balance(req.account_num);
                    delete_sent(req);//11.9
                    if (req.type == "transfer" && bank_id == req.dest_bank_id)
                    {
                        //i am the dest bank's head&tail, reply to client,
                        //send ack to source bank
                        ack_to_source_bank(req);
                    }
                    else
                    {
                        reply_to_client(req, rpy);//11.20 dest bank don't reply to client
                    }
                }
            }
        }
    }
}
/*reply result to client*/
void reply_to_client(request_struct req, reply_struct rpy)
{
    send_num++;
    check_my_life();
    fprintf(out, "(%-24s) REPLY:  reqID:%-8s accountNum:%-6d outcome:%-26s balance:%-8.2f\n",
            get_time(), rpy.req_id.c_str(), rpy.account_num, Outcome[rpy.outcome].c_str(), rpy.balance);
    fflush(out);

    client_addr.sin_addr.s_addr = inet_addr(req.client_ip);//client ip
    client_addr.sin_port = htons(req.client_port);//client port

    char buf[MAXDATASIZE];
    buf[0] = SER_RPY; //OPCODE

    memcpy(&buf[1], &rpy, sizeof(rpy));

    sendto(reply_client_fd, buf, MAXDATASIZE,
           0, (struct sockaddr *)&client_addr, sizeof(client_addr));
}
//=========================================
void init_udp_client()
{
    bzero(&client_addr, sizeof(client_addr));
    client_addr.sin_family = AF_INET;

    if ((reply_client_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Error opening socket\n");
        exit(1);
    }
}