#include "client.h"
#include "client_read_conf.h"

/* extern var */
int master_port;
char master_ip[16];

int head_port;
int tail_port;
int my_port;
char my_ip[16];
char head_ip[16];
char tail_ip[16];
int client_id;
string bank_name;
int bank_id;
int wait_time;
int resend_times;
bool whether_resend_newhead;
queue <request_struct> req_que;

int send_head_fd, send_tail_fd;
int send_master_fd;

struct sockaddr_in ser_addr;
struct sockaddr_in ma_addr;

pthread_t time_thread;
pthread_t rec_thread;

bool recv_table[200];
int last_rec_id;

FILE *out;

/* end extern var */
void send_tail_server(request_struct req)
{
    ser_addr.sin_addr.s_addr = inet_addr(tail_ip);//tail ip
    ser_addr.sin_port = htons(tail_port);

    char buf[MAXDATASIZE];
    /*change struct to char**/
    memcpy(buf, &req, sizeof(req));

    sendto(send_tail_fd, buf, MAXDATASIZE, 0, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
}

void send_head_server(request_struct req)
{
    ser_addr.sin_addr.s_addr = inet_addr(head_ip);//head ip
    ser_addr.sin_port = htons(head_port);

    char buf[MAXDATASIZE];
    /*change struct to char**/
    memcpy(buf, &req, sizeof(req));

    sendto(send_head_fd, buf, MAXDATASIZE, 0, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
}
/*check whether has received reply from server*/
void *check_receive(void *args)
{
    request_struct *req_p = (request_struct *)args;
    request_struct req;
    /*copy struct*/
    req.req_seq = 0;
    req.sent_id = req_p->sent_id;
    strcpy(req.client_ip, req_p->client_ip);
    req.client_port = req_p->client_port;
    req.type = req_p->type;
    req.req_id = req_p->req_id;
    req.account_num = req_p->account_num;
    req.amount = req_p->amount;
    req.status = req_p->status;

    int my_resend_time = 0;//< resend_times
    int timer = 0;// < wait_time
    while (1)
    {
        if (recv_table[req.sent_id] || my_resend_time == resend_times || !whether_resend_newhead)
            break;

        if (timer >= wait_time && last_rec_id == req.sent_id - 1)//11.8
        {
            if (req.type == "getBalance") //query
            {
                send_tail_server(req);//resend
            }
            else if (req.type == "deposit" || req.type == "withdraw" || req.type == "transfer") //update
            {
                send_head_server(req);//resend
            }
            my_resend_time++;
            printf("resend #%d: %s\n", req.sent_id, req.req_id.c_str());
            fprintf(out, "(%-24s) resend #%d:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f\n",
                    get_time(), req.sent_id, req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount);
            fflush(out);
        }
        timer++;
        sleep(wait_time);
    }
}
/*UDP: receive reply from the tail server, and master's message(set new head or tail)*/
void *recv_msg(void *args)
{
    int sockfd;
    char buf[MAXDATASIZE];

    struct sockaddr_in cli_addr;
    bzero(&cli_addr, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = inet_addr(my_ip);
    cli_addr.sin_port = htons(my_port);
    socklen_t cli_len = sizeof(cli_addr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("error opening socket\n");
        exit(1);
    }
    if (bind(sockfd, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) == -1)
    {
        printf("error in bind\n");
        exit(1);
    }

    while (1)
    {
        if (recvfrom(sockfd, buf, MAXDATASIZE, 0, (struct sockaddr *)&cli_addr, &cli_len) > 0)
        {

            if (buf[0] == SER_RPY) /*get reply from tail server*/
            {
                reply_struct rpy;
                /*get reply from tail server, change char* to struct*/
                memcpy(&rpy, &buf[1], sizeof(buf) - sizeof(char));

                recv_table[rpy.sent_id] = true;// get received
                last_rec_id ++;

                printf("recv #%d: %s\n", rpy.sent_id, rpy.req_id.c_str());

                fprintf(out, "(%-24s) recv #%d:  reqID:%-8s accountNum:%-6d outcome:%-26s balance:%-8.2f\n",
                        get_time(), rpy.sent_id, rpy.req_id.c_str(), rpy.account_num, Outcome[rpy.outcome].c_str(), rpy.balance);
                fflush(out);

                send_request();
            }
            else if (buf[0] == SET_HEAD_TAIL) //from master's message: set new head and tail
            {
                set_head_tail sht;

                memcpy(&sht, &buf[1], sizeof(buf) - sizeof(char));
                //set new head an tail
                if (bank_id == sht.bank_id)
                {
                    head_port = sht.head_port;
                    strcpy(head_ip, sht.head_ip);
                    tail_port = sht.tail_port;
                    strcpy(tail_ip, sht.tail_ip);
                    printf("bank#%d head:%d, tail:%d\n", sht.bank_id, head_port, tail_port);
                    fprintf(out, "(%-24s) bank#%d head:%d, tail:%d\n", get_time(), sht.bank_id, head_port, tail_port);
                    fflush(out);
                }
            }
        }
    }
}

/*================= client with master =======================*/
void send_master()
{
    ma_addr.sin_addr.s_addr = inet_addr(master_ip);//tail ip
    ma_addr.sin_port = htons(master_port);

    client_master_struct cms;

    cms.bank_id = bank_id;
    strcpy(cms.client_ip, my_ip);
    cms.client_port = my_port;

    char buf[MAXDATASIZE];
    /*change struct to char**/
    memcpy(buf, &cms, sizeof(cms));

    sendto(send_master_fd, buf, MAXDATASIZE, 0, (struct sockaddr *)&ma_addr, sizeof(ma_addr));
}

/*================== end client with master ===================*/

void init_client(int bankid, int clientid)
{
    if (bankid < 1 || clientid < 1)
    {
        printf("illegal client info input\n");
        return;
    }
    bank_id = bankid;
    client_id = clientid;

    /*get the current client's information from the config file */
    /* in client_read_conf.cpp */
    get_client_need_info();

    /* init server address */
    bzero(&ser_addr, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;

    /* init master address */
    bzero(&ma_addr, sizeof(ma_addr));
    ma_addr.sin_family = AF_INET;

    if ((send_head_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Error opening head_fd socket\n");
        exit(1);
    }
    if ((send_tail_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Error opening tail_fd socket\n");
        exit(1);
    }
    if ((send_master_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Error opening master_fd socket\n");
        exit(1);
    }
    /*when client login, tell master its own bank_id, ip, port
    to get head, tail servers' info*/
    send_master();
}

int main(int argc, char *argv[])//bank_id  client_id
{
    if (argc != 3)
    {
        printf("input error\n");
        exit(1);
    }
    memset(recv_table, false, 200 * sizeof(bool));
    last_rec_id = 0;

    /*init client and get client's information from the config file */
    init_client(atoi(argv[1]), atoi(argv[2]));
    string q1 = argv[1], q2 = argv[2];

    string my_log_file = "../../logs/bank" + q1 + "/client-" + q2 + ".txt";
    out = fopen(my_log_file.c_str(), "w+" );

    pthread_create(&rec_thread, NULL, recv_msg, NULL);

    sleep(1);//wait for master reply
    send_request();

    pthread_join(rec_thread, NULL);

    fclose(out);

    return 0;
}/*main*/

void send_request()
{
    if (req_que.empty())
        return;

    request_struct req = req_que.front();

    if (last_rec_id != req.sent_id - 1)
        return;

    req_que.pop();
    pthread_create(&time_thread, NULL, check_receive, &req);

    if (req.type == "getBalance")
    {
        send_tail_server(req);

        fprintf(out, "(%-24s) send #%d:  type:%-10s reqID:%-8s accountNum:%-6d\n",
                get_time(), req.sent_id, req.type.c_str(), req.req_id.c_str(), req.account_num);
        fflush(out);
    }
    else if (req.type == "transfer")
    {
        send_head_server(req);

        fprintf(out, "(%-24s) send #%d:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f destBank:%-3d destAccount:%-6d\n",
                get_time(), req.sent_id, req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount, req.dest_bank_id, req.dest_account_num);
        fflush(out);
    }
    else//deposit & withdraw
    {
        send_head_server(req);

        fprintf(out, "(%-24s) send #%d:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f\n",
                get_time(), req.sent_id, req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount);
        fflush(out);
    }
    usleep(500000);
}



