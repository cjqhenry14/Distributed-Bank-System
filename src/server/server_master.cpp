#include "server.h"
#include "server_master.h"
#include "server_client.h"

/*listen server's socket*/
void listen_next_again()
{
    printf("listening...\n");

    /*receive next server connect*/
    if ((pthread_create(&acc_next_thread, NULL, accept_connect_from_next_server, NULL)) != 0)
    {
        printf("create accept thread error!\n");
        exit(1);
    }
}
/*TCP: tell master i am the new tail, for extend*/
void tell_master_newtail(master_server_struct mss)
{
    char msg[MAXDATASIZE];
    msg[0] = NEW_TAIL; //OPCODE

    memcpy(&msg[1], &mss, sizeof(mss));

    if (send(masterfd, msg, sizeof(msg), 0) == -1)
    {
        perror("send");
        exit(1);
    }
}
/*TCP: send msg to master*/
void send_to_master(int last_sn)
{
    char msg[2];
    msg[0] = ACK_LAST; //OPCODE
    msg[1] = last_sn;//last_sn

    if (send(masterfd, msg, sizeof(msg), 0) == -1)
    {
        perror("send");
        exit(1);
    }
}
/*TCP: receive msg from master*/
void *rec_msg_from_master(void *)
{
    char buf[MAXDATASIZE];
    int numbytes;
    while (1)
    {
        if ((numbytes = recv(masterfd, buf, MAXDATASIZE, 0)) == -1) {}
        buf[numbytes] = '\0';
        if (strlen(buf) != 0)
        {
            if (buf[0] == NEXT_SN)
            {
                sn_struct sns;
                memcpy(&sns, &buf[1], sizeof(buf) - sizeof(char));
                usleep(20000);//20ms
                /*send fail next request.sent_id larger than sn*/
                send_next_larger_sn(sns.last_sn);
            }
            else if (buf[0] == GET_PRE && my_type != HEAD && my_type != HEAD_TAIL) //get pre server from master, then connect to pre
            {
                master_server_struct mss;
                memcpy(&mss, &buf[1], sizeof(buf) - sizeof(char));
                printf("pre ser:%d\n", mss.server_port);
                pre_port = mss.server_port;
                strcpy(pre_ip, mss.server_ip);

                pthread_create(&conn_pre_thread, NULL, connect_pre_server, NULL);
            }
            else if (buf[0] == HEAD_FAIL)//my pre(head) fail, set myself = (HEAD | HEAD_TAIL)
            {
                if (my_type == TAIL)
                    my_type = HEAD_TAIL;
                else
                    my_type = HEAD;
                /*stop receive from pre*/
                pthread_cancel(rec_pre_thread);
                /*break the connection with pre*/
                close(pre_fd);
                connected_pre = false;

                printf("pre(head):%d died, I am new head\n", pre_port);
                fprintf(out, "(%-24s) pre(head):%d died, I am new head\n", get_time(), pre_port);
                fflush(out);
            }
            else if (buf[0] == TAIL_FAIL)//my next(tail) fail, set myself = (TAIL | HEAD_TAIL)
            {
                if (my_type == HEAD)
                    my_type = HEAD_TAIL;
                else
                    my_type = TAIL;
                /*stop receive from next*/
                pthread_cancel(rec_next_thread);

                pthread_cancel(acc_next_thread);
                /*break the connection with next*/
                close(next_fd);
                connected_next = false;

                printf("next(tail):%d died, I am new tail\n", next_port);
                fprintf(out, "(%-24s) next(tail):%d died, I am new tail\n", get_time(), next_port);
                fflush(out);

                listen_next_again();
            }
            else if (buf[0] == INTER_PRE_FAIL)//my pre(inter) fail
            {
                master_server_struct mss;
                memcpy(&mss, &buf[1], sizeof(buf) - sizeof(char));

                printf("pre:%d died, new pre:%d\n", pre_port, mss.server_port);
                fprintf(out, "(%-24s) pre:%d died, new pre:%d\n", get_time(), pre_port, mss.server_port);
                fflush(out);
                //set new pre
                pre_port = mss.server_port;
                strcpy(pre_ip, mss.server_ip);
                /*stop receive from pre*/
                pthread_cancel(rec_pre_thread);
                /*break the connection with pre*/
                close(pre_fd);
                connected_pre = false;
                /*connect new pre server*/
                pthread_create(&conn_pre_thread, NULL, connect_pre_server, NULL);
                /*ack to master with the last sequence number in sent*/
                //send_to_master(sent.back().sent_id);
                send_to_master(last_received_id);
            }
            else if (buf[0] == INTER_NEXT_FAIL)//my next(inter) fail
            {
                master_server_struct mss;
                memcpy(&mss, &buf[1], sizeof(buf) - sizeof(char));

                printf("next:%d died, new next:%d\n", next_port, mss.server_port);
                fprintf(out, "(%-24s) next:%d died, new next:%d\n", get_time(), next_port, mss.server_port);
                fflush(out);
                //set new next
                next_port = mss.server_port;
                strcpy(next_ip, mss.server_ip);
                /*stop receive from next*/
                pthread_cancel(rec_next_thread);

                pthread_cancel(acc_next_thread);
                /*break the connection with next*/
                close(next_fd);
                connected_next = false;

                listen_next_again();
            }
            else if (buf[0] == TRANS_HEAD_FAIL)//other bank's head fail 11.20
            {
                transfer_head_fail_struct thfs;

                memcpy(&thfs, &buf[1], sizeof(buf) - sizeof(char));
                all_heads[thfs.fail_bank_id - 1] = thfs.new_head_port;

                printf("bank#%d's head died, new head:%d\n", thfs.fail_bank_id, thfs.new_head_port);
                fprintf(out, "(%-24s) bank#%d's head died, new head:%d\n", get_time(), thfs.fail_bank_id, thfs.new_head_port);
                fflush(out);

                transfer_resend_head_fail(thfs);
            }
            else if (buf[0] == TRANS_TAIL_FAIL)//other bank's tail fail 11.20
            {
                transfer_tail_fail_struct ttfs;

                memcpy(&ttfs, &buf[1], sizeof(buf) - sizeof(char));
                all_tails[ttfs.fail_bank_id - 1] = ttfs.new_tail_port;

                printf("bank#%d's tail died, new tail:%d\n", ttfs.fail_bank_id, ttfs.new_tail_port);
                fprintf(out, "(%-24s) bank#%d's tail died, new tail:%d\n", get_time(), ttfs.fail_bank_id, ttfs.new_tail_port);
                fflush(out);
            }
            //else if()other msg type from next server
            memset(buf, 0, MAXDATASIZE);
        }
    }/*while*/
}

/*UDP: round_ask to master*/
void *round_ask(void *)
{
    int round_ask_fd;
    if ((round_ask_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Error opening round_ask_fd socket\n");
        exit(1);
    }

    struct sockaddr_in round_addr;
    bzero(&round_addr, sizeof(round_addr));
    round_addr.sin_family = AF_INET;
    round_addr.sin_addr.s_addr = inet_addr(master_ip);//tail ip
    round_addr.sin_port = htons(master_port + 100);

    char buf[MAXDATASIZE];
    master_server_struct mss;
    mss.bank_id = bank_id;
    mss.server_port = my_port;
    memcpy(&buf, &mss, sizeof(mss));

    while (1)
    {
        sendto(round_ask_fd, buf, MAXDATASIZE, 0, (struct sockaddr *)&round_addr, sizeof(round_addr));
        sleep(1);
    }
}

/*connect to master*/
void *connect_master(void *)
{
    struct hostent *he;
    /*get pre server's ip*/
    if ((he = gethostbyname(master_ip)) == NULL)
    {
        perror("gethostbyname");
        exit(1);
    }
    /*create socket*/
    if ((masterfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }
    /*init sockaddr_in struct*/
    struct sockaddr_in their_addr;
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(master_port);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    bzero(&(their_addr.sin_zero), 8);
    /*send connect signal to pre server*/
    if (connect(masterfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("connect");
        exit(1);
    }

    printf("got connection with master\n");
    fprintf(out, "(%-24s) got connection with master\n", get_time());
    fflush(out);

    if ((pthread_create(&round_ask_thread, NULL, round_ask, NULL)) != 0)
    {
        printf("create thread error!\r\n");
        exit(1);
    }

    /*send server's bank_id, ip, port*/
    char buf[MAXDATASIZE];

    master_server_struct mss;
    mss.bank_id = bank_id;
    mss.server_port = my_port;
    strcpy(mss.server_ip, my_ip);

    memcpy(&buf, &mss, sizeof(mss));
    send(masterfd , buf, sizeof(buf), 0);

    if ((pthread_create(&rec_master_thread, NULL, rec_msg_from_master, NULL)) != 0)
    {
        printf("create thread error!\r\n");
        exit(1);
    }

    extend_new_fail = true;
    check_my_life();
}