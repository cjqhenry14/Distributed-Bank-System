#include "master.h"
#include "master_read_conf.h"
/* extern var */
int master_port;
char master_ip[16];
//========
vector<bank_struct> bank_vector;
pthread_t rec_cli_thread, acc_ser_thread, rec_ser_thread, round_ask_thread;
pthread_t round_ask_rec_thread, round_ask_check_thread;

struct sockaddr_in ma_addr;
socklen_t ma_len;
//========
struct sockaddr_in cli_addr;

int send_cli_fd, maserfd;
int fail_pre, fail_next, fail_pre_fd;
int tmp = 0;

FILE *out;
/*================= master with client =======================*/
void send_client(set_head_tail sht, client_master_struct cms)
{
    cli_addr.sin_addr.s_addr = inet_addr(cms.client_ip);//tail ip
    cli_addr.sin_port = htons(cms.client_port );

    char buf[MAXDATASIZE];
    buf[0] = SET_HEAD_TAIL;

    memcpy(&buf[1], &sht, sizeof(sht));

    sendto(send_cli_fd, buf, MAXDATASIZE, 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
}
/*multicast to client set new head and tail*/
void multicast_client(int bank_id)
{
    list<client_struct>::iterator it;
    int i = bank_id - 1;
    for (it = bank_vector[i].client_list.begin(); it != bank_vector[i].client_list.end(); it++)
    {
        master_server_struct head_server = get_head_server(bank_id);
        master_server_struct tail_server = get_tail_server(bank_id);

        set_head_tail sht;
        sht.bank_id = bank_id;
        sht.head_port = head_server.server_port;
        strcpy(sht.head_ip, head_server.server_ip);
        sht.tail_port = tail_server.server_port;
        strcpy(sht.tail_ip, tail_server.server_ip);

        client_master_struct cms;
        cms.client_port = it->client_port;
        strcpy(cms.client_ip, it->client_ip);

        send_client(sht, cms);
    }
}
/*udp: receive from client*/
void *recv_client(void *args)
{
    int sockfd;
    char buf[MAXDATASIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("error opening socket\n");
        exit(1);
    }
    if (bind(sockfd, (struct sockaddr *)&ma_addr, sizeof(ma_addr)) == -1)
    {
        printf("error in bind\n");
        exit(1);
    }

    while (1)
    {
        if (recvfrom(sockfd, buf, MAXDATASIZE, 0, (struct sockaddr *)&ma_addr, &ma_len) > 0)
        {
            client_master_struct cms;
            /*get reply from tail server*/
            memcpy(&cms, buf, sizeof(buf));

            master_server_struct head_server = get_head_server(cms.bank_id);
            master_server_struct tail_server = get_tail_server(cms.bank_id);

            set_head_tail sht;
            sht.bank_id = cms.bank_id;
            sht.head_port = head_server.server_port;
            strcpy(sht.head_ip, head_server.server_ip);
            sht.tail_port = tail_server.server_port;
            strcpy(sht.tail_ip, tail_server.server_ip);

            printf("BANK#%d CLIENT:%d SET HEAD:%d & TAIL:%d\n", cms.bank_id, cms.client_port, head_server.server_port, tail_server.server_port);
            fprintf(out, "(%-24s) BANK#%d CLIENT:%d SET HEAD:%d & TAIL:%d\n",
                    get_time(), cms.bank_id, cms.client_port, head_server.server_port, tail_server.server_port);
            fflush(out);

            usleep(200);//sleep for 0.2ms
            send_client(sht, cms);
            usleep(200);//sleep for 0.2ms

            /*clean the buf*/
            memset(buf, 0, MAXDATASIZE);
        }
    }
}

/*================= master with server =======================*/
/*head fail, tell other banks' tails*/
void transfer_head_fail(int fail_bank_id, int new_head_port)
{
    char msg[MAXDATASIZE];
    msg[0] = TRANS_HEAD_FAIL;

    transfer_head_fail_struct thfs;
    thfs.fail_bank_id = fail_bank_id;
    thfs.new_head_port = new_head_port;

    memcpy(&msg[1], &thfs, sizeof(thfs));

    for (int k = 0; k < bank_vector.size(); ++k)
    {
        if (!bank_vector[k].server_list.empty() && k != fail_bank_id - 1)
        {
            int skfd = bank_vector[k].server_list.back().sockfd;
            printf("tell bank#%d's tail that bank#%d's head died\n", k + 1, fail_bank_id);
            fprintf(out, "(%-24s) tell bank#%d's tail that bank#%d's head died\n", get_time(), k + 1, fail_bank_id);
            fflush(out);
            if (send(skfd, msg, sizeof(msg), 0) == -1)
                perror("send");
        }
    }
}
/*tail fail, tell other banks' heads*/
void transfer_tail_fail(int fail_bank_id, int new_tail_port)
{
    char msg[MAXDATASIZE];
    msg[0] = TRANS_TAIL_FAIL;

    transfer_tail_fail_struct ttfs;

    ttfs.fail_bank_id = fail_bank_id;
    ttfs.new_tail_port = new_tail_port;

    memcpy(&msg[1], &ttfs, sizeof(ttfs));

    for (int k = 0; k < bank_vector.size(); ++k)
    {
        if (!bank_vector[k].server_list.empty() && k != fail_bank_id - 1)
        {
            printf("tell bank#%d's head that bank#%d's tail died\n", k + 1, fail_bank_id);
            fprintf(out, "(%-24s) tell bank#%d's head that bank#%d's tail died\n", get_time(), k + 1, fail_bank_id);
            fflush(out);

            list<server_struct>::iterator it;
            for (it = bank_vector[k].server_list.begin(); it != bank_vector[k].server_list.end(); it++)
            {
                int skfd = it->sockfd;

                if (send(skfd, msg, sizeof(msg), 0) == -1)
                    perror("send");
                usleep(20000);
            }
        }
    }
}
void server_fail_notice(int serfd, master_server_struct mss, char fail_type)
{
    char msg[MAXDATASIZE];
    msg[0] = fail_type;

    memcpy(&msg[1], &mss, sizeof(mss));

    if (send(serfd, msg, sizeof(msg), 0) == -1)
    {
        perror("send");
    }
}
void tell_server_pre(int serfd, master_server_struct pre_ser)
{
    fprintf(out, "(%-24s) current tail is:%d\n", get_time(), pre_ser.server_port);
    fflush(out);

    char msg[MAXDATASIZE];
    msg[0] = GET_PRE;

    memcpy(&msg[1], &pre_ser, sizeof(pre_ser));

    if (send(serfd, msg, sizeof(msg), 0) == -1)
    {
        perror("send");
    }
}
void send_sn_to_pre(int serfd, sn_struct sns)
{
    char msg[MAXDATASIZE];
    msg[0] = NEXT_SN;//tell fail pre to send request.sent_id larger than sn
    memcpy(&msg[1], &sns, sizeof(sns));

    if (send(serfd, msg, sizeof(msg), 0) == -1)
    {
        perror("send");
    }
}
void *round_ask_check(void *)
{
    while (1)
    {
        list<server_struct>::iterator it, pre, next, temp;
        for (int i = 0; i < bank_vector.size(); ++i)
        {
            for (it = bank_vector[i].server_list.begin(); it != bank_vector[i].server_list.end();)
            {
                if (it->round_flag == LIVE)
                {
                    it->round_flag = DIE;
                    pre = it;
                    it++;
                }
                else//DIE
                {
                    if (bank_vector[i].server_list.size() == 1)
                    {
                        printf("server:%d died, no other servers\n", it->server_port);
                        fprintf(out, "(%-24s) server:%d died, no other servers\n", get_time(), it->server_port);
                        fflush(out);

                        it = bank_vector[i].server_list.erase(it);
                        printf("after delete\n");
                        show_server_list();
                    }
                    else if (it == bank_vector[i].server_list.begin())//head fail  11.20
                    {
                        temp = it;
                        it = bank_vector[i].server_list.erase(it);
                        printf("after delete\n");
                        show_server_list();

                        master_server_struct mss;
                        server_fail_notice(it->sockfd, mss, HEAD_FAIL);
                        printf("head server:%d died, next:%d\n", temp->server_port, it->server_port);

                        fprintf(out, "(%-24s) head server:%d died, next:%d\n", get_time(), temp->server_port, it->server_port);
                        fflush(out);

                        //11.20
                        usleep(30000);
                        transfer_head_fail(i + 1, it->server_port); //fail_bank_id, new_head
                    }
                    else if (it->server_port == bank_vector[i].server_list.back().server_port)
                    {
                        temp = it;
                        it = bank_vector[i].server_list.erase(it);
                        printf("after delete\n");
                        show_server_list();

                        master_server_struct mss;
                        server_fail_notice(pre->sockfd, mss, TAIL_FAIL);
                        printf("tail server:%d died, pre:%d\n", temp->server_port, pre->server_port);

                        fprintf(out, "(%-24s) tail server:%d died, pre:%d\n", get_time(), temp->server_port, pre->server_port);
                        fflush(out);

                        //11.20
                        transfer_tail_fail(i + 1, pre->server_port); //fail_bank_id, new_tail
                    }
                    else
                    {
                        temp = it;

                        it = bank_vector[i].server_list.erase(it);
                        printf("after delete\n");
                        show_server_list();

                        master_server_struct mss;
                        mss.server_port = it->server_port;//tell cur's pre the new next port
                        strcpy(mss.server_ip, it->server_ip);//tell cur's pre the new next ip
                        server_fail_notice(pre->sockfd, mss, INTER_NEXT_FAIL);

                        mss.server_port = pre->server_port;//tell cur's next the new pre port
                        strcpy(mss.server_ip, pre->server_ip);//tell cur's next the new pre ip
                        server_fail_notice(it->sockfd, mss, INTER_PRE_FAIL);

                        fail_pre = pre->server_port, fail_next = it->server_port;
                        fail_pre_fd = pre->sockfd;

                        printf("inter server:%d died, pre:%d, next:%d\n", temp->server_port, pre->server_port, it->server_port);

                        fprintf(out, "(%-24s) inter server:%d died, pre:%d, next:%d\n",
                                get_time(), temp->server_port, pre->server_port, it->server_port);
                        fflush(out);
                    }

                    /*multicast to client the server's death and set new head and tail*/
                    multicast_client(i + 1);
                }
            }
        }
        sleep(3);
    }
}
/*udp: round_ask_receive to each server*/
void *round_ask_receive(void *)
{
    int roundfd;

    char buf[MAXDATASIZE];
    struct sockaddr_in round_addr;
    bzero(&round_addr, sizeof(round_addr));
    round_addr.sin_family = AF_INET;
    round_addr.sin_addr.s_addr = inet_addr(master_ip);//my ip
    round_addr.sin_port = htons(master_port + 100);// another port to
    socklen_t round_len = sizeof(round_addr);

    if ((roundfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("error opening socket\n");
        exit(1);
    }
    if (bind(roundfd, (struct sockaddr *)&round_addr, sizeof(round_addr)) == -1)
    {
        printf("error in bind\n");
        exit(1);
    }
    while (1)
    {
        if (recvfrom(roundfd, buf, MAXDATASIZE, 0, (struct sockaddr *)&round_addr, &round_len) > 0)
        {
            master_server_struct mss;

            memcpy(&mss, &buf, sizeof(buf));
            //printf("round ask from: %d\n", mss.server_port);
            update_round_flag(mss.bank_id, mss.server_port, LIVE);

            memset(buf, 0, MAXDATASIZE);
        }
    }
}
/*TCP: receive msg from each server*/
void *rec_msg_from_server(void *arg)
{
    int serfd = *(int *)arg;

    char buf[MAXDATASIZE];
    int numbytes;
    while (1)
    {
        if ((numbytes = recv(serfd, buf, MAXDATASIZE, 0)) == -1) {}
        buf[numbytes] = '\0';
        if (strlen(buf) != 0)
        {
            if (buf[0] == ACK_LAST)
            {
                int last_sn = buf[1];
                memset(buf, 0, MAXDATASIZE);
                printf("%d should send %d with greater than sn:%d\n", fail_pre, fail_next, last_sn);
                fprintf(out, "(%-24s) %d should send %d with greater than sn:%d\n",
                        get_time(), fail_pre, fail_next, last_sn);
                fflush(out);
                /*Master sends S- a notification that S crashed containing sn*/
                sn_struct sns;
                sns.last_sn = last_sn;
                send_sn_to_pre(fail_pre_fd, sns);
            }
            else if (buf[0] == NEW_TAIL)
            {
                master_server_struct mss;
                memcpy(&mss, &buf[1], sizeof(buf) - sizeof(char));
                memset(buf, 0, MAXDATASIZE);
                server_struct ss;
                ss.round_flag = LIVE;
                ss.sockfd = serfd;
                ss.bank_id = mss.bank_id;
                ss.server_port = mss.server_port;
                strcpy(ss.server_ip, mss.server_ip);

                bank_vector[ss.bank_id - 1].server_list.push_back(ss);
                /*when new server get in, tell clients new tail*/
                show_server_list();
                multicast_client(mss.bank_id);
            }
        }
    }/*while*/
}
/*accept connect from server*/
void *accept_connect_from_server(void *)
{
    struct sockaddr_in their_addr;
    socklen_t sin_size = sizeof(struct sockaddr_in);
    int serfd;

    while (1)
    {
        if ((serfd = accept(maserfd, (struct sockaddr *) &their_addr, &sin_size)) < 0)
        {
            perror("accept");
            exit(1);
        }
        char buf[MAXDATASIZE];
        recv(serfd, buf, MAXDATASIZE, 0);

        master_server_struct mss;
        memcpy(&mss, &buf, sizeof(buf));

        printf("get connection with server: %d\n", mss.server_port);
        fprintf(out, "(%-24s) get connection with server: %d\n", get_time(), mss.server_port);
        fflush(out);

        //11.20if (tmp == 0) //first server
        if (mss.server_port % 10 == 1)
        {
            server_struct ss;
            ss.round_flag = LIVE;
            ss.sockfd = serfd;
            ss.bank_id = mss.bank_id;
            ss.server_port = mss.server_port;
            strcpy(ss.server_ip, mss.server_ip);

            bank_vector[ss.bank_id - 1].server_list.push_back(ss);
            show_server_list();
            multicast_client(mss.bank_id);
        }

        master_server_struct pre_ser = get_tail_server(mss.bank_id);
        /*tell the new server its previousu*/
        tell_server_pre(serfd, pre_ser);
        /*when new server get in, tell clients new tail*/
        //multicast_client(mss.bank_id);11.10  should not be here
        //show_server_list();should not be here

        /*receive server*/
        if ((pthread_create(&rec_ser_thread, NULL, rec_msg_from_server, &serfd)) != 0)
        {
            printf("create accept thread error!\n");
            exit(1);
        }
    }
}

void init_listen_server()
{
    if ((maserfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }
    /*init sockaddr_in*/
    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(master_port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(my_addr.sin_zero), 8);

    /*bind socket and port*/
    if (bind(maserfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }
    /*listen client's socket*/
    if (listen(maserfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }
    printf("listening...\n");

    /*receive server connect*/
    if ((pthread_create(&acc_ser_thread, NULL, accept_connect_from_server, NULL)) != 0)
    {
        printf("create accept thread error!\n");
        exit(1);
    }
}

/*================= master self =======================*/

/*init master socket address struct*/
void init_master_add()
{
    bzero(&ma_addr, sizeof(ma_addr));
    ma_addr.sin_family = AF_INET;
    ma_addr.sin_addr.s_addr = inet_addr(master_ip);
    ma_addr.sin_port = htons(master_port);
    ma_len = sizeof(ma_addr);
}

void init_master()
{
    /*get the current master's information from the config file */
    /* in master_read_conf.cpp */
    get_master_need_info();
    /*ignore broken pipe*/
    signal(SIGPIPE, SIG_IGN);

    string my_log_file = "../../logs/master.txt";
    out = fopen(my_log_file.c_str(), "w+" );

    init_master_add();

    /* init client address */
    bzero(&cli_addr, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;

    if ((send_cli_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Error opening send_cli_fd socket\n");
        exit(1);
    }

    init_listen_server();
}


int main(int argc, char *argv[])
{
    /*init master and get master's information from the config file */
    init_master();

    pthread_create(&rec_cli_thread, NULL, recv_client, NULL);
    pthread_create(&round_ask_rec_thread, NULL, round_ask_receive, NULL);
    pthread_create(&round_ask_check_thread, NULL, round_ask_check, NULL);

    pthread_join(rec_cli_thread, NULL);
    pthread_join(acc_ser_thread, NULL);
    pthread_join(round_ask_rec_thread, NULL);
    pthread_join(round_ask_check_thread, NULL);

    return 0;
}/*main*/


/******************** other basic function *****************************/
void update_round_flag(int bank_id, int server_port, int round_flag)
{
    list<server_struct>::iterator it;
    int i = bank_id - 1;
    for (it = bank_vector[i].server_list.begin(); it != bank_vector[i].server_list.end(); it++)
    {
        if (it->server_port == server_port)
        {
            it->round_flag = round_flag;
            return;
        }
    }
}

void show_server_list()
{
    /*show each bank's server(ip, port)*/
    for (int i = 0; i < bank_vector.size(); ++i)
    {
        printf("bank# %d: size:%d | server: ", i + 1, (int)bank_vector[i].server_list.size());
        for (list<server_struct>::iterator it1 = bank_vector[i].server_list.begin(); it1 != bank_vector[i].server_list.end(); it1++)
        {
            printf(" %d ", it1->server_port);
        }
        printf("\n");
    }
}

master_server_struct get_head_server(int bank_id)
{
    master_server_struct mss;
    server_struct ss = bank_vector[bank_id - 1].server_list.front();

    strcpy(mss.server_ip, ss.server_ip);
    mss.server_port = ss.server_port;

    return mss;
}
master_server_struct get_tail_server(int bank_id)
{
    master_server_struct mss;
    server_struct ss = bank_vector[bank_id - 1].server_list.back();

    mss.bank_id = ss.bank_id;
    mss.server_port = ss.server_port;
    strcpy(mss.server_ip, ss.server_ip);
    return mss;
}

