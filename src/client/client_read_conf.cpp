#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filestream.h"

#include <cstdio>
#include <iostream>
#include <string>

#include "client.h"
#include "client_read_conf.h"

using namespace rapidjson;
using namespace std;
/*get the
bank:(name, chain_length)
server:(ip, port)
client:(ip, port, wait_time, resend_times, whether_resend_newhead)
request:(getBalance, reqID, accountNum), (deposit, reqID, accountNum, amount)
we have got [bank_id, client_id]
*/
void show_request()
{
    while (!req_que.empty())
    {
        request_struct req = req_que.front();
        req_que.pop();
        printf("<%s, %s, %d, %.2f>\n", req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount);
    }
}
void show_info()
{
    cout << "BANK # " << bank_id << endl;
    cout << "bank_name: " << bank_name << endl;
    cout << "HEAD IP: " << head_ip << " ";
    cout << "HEAD PORT: " << head_port << " ";
    cout << endl;
    cout << "tail IP: " << tail_ip << " ";
    cout << "tail PORT: " << tail_port << " ";
    cout << endl;

    cout << "MY IP: " << my_ip << " ";
    cout << "MY PORT: " << my_port << " ";
    cout << "wait_time: " << wait_time << " ";
    cout << "resend_times: " << resend_times << " ";
    cout << "whether_resend_newhead: " << whether_resend_newhead << " ";
    cout << "----request queue----" << endl;
    while (!req_que.empty())
    {
        request_struct req = req_que.front();
        req_que.pop();
        cout << "type: " << req.type << endl;
        cout << "client_ip: " << req.client_ip << endl;
        cout << "client_port: " << req.client_port << endl;
        cout << "req_id: " << req.req_id << endl;
        cout << "account_num: " << req.account_num << endl;
        cout << "amount: " << req.amount << endl;
        cout << "**********" << endl;
    }
}
void get_client_need_info()
{
    FILE *fp = fopen(CONFIG_FILE, "r");
    FileStream is(fp);

    Document bank;
    bank.ParseStream<0>(is);

    if (bank.HasParseError())
    {
        printf("GetParseError %s\n", bank.GetParseError());
        return;
    }
    /*get master info(ip, port)*/
    unsigned int banks_num = bank.Size();
    const Value &master = bank[banks_num - 1]; //master info
    strcpy(master_ip, master["master_ip"].GetString());
    master_port = master["master_port"].GetInt();

    /*get bank basic info*/
    const Value &cur_bank = bank[bank_id - 1];
    bank_name = cur_bank["bank_name"].GetString();
    int chain_length = cur_bank["chain_length"].GetInt();

    /*get client's self info (ip, port, wait_time, resend_times, whether_resend_newhead) */
    unsigned int client_i = client_id - 1;
    const Value &cur_client = cur_bank["client"][client_i];
    strcpy(my_ip, cur_client["ip"].GetString());
    my_port = cur_client["port"].GetInt();
    wait_time = cur_client["wait_time"].GetInt();
    resend_times = cur_client["resend_times"].GetInt();
    whether_resend_newhead = cur_client["whether_resend_newhead"].GetBool();

    /*get client's request queue*/
    const Value &request_array = cur_client["request"];
    int sent_id = 1;
    for (unsigned int k = 0; k < request_array.Size(); ++k)
    {
        const Value &cur_request = request_array[k];

        if (cur_request.HasMember("type"))
        {
            request_struct req;
            req.sent_id = sent_id;//11.8 
            sent_id++;
            strcpy(req.client_ip, my_ip);
            req.client_port = my_port;
            req.type = cur_request["type"].GetString();
            req.req_id = cur_request["reqID"].GetString();
            req.account_num = cur_request["accountNum"].GetInt();
            if (req.type == "getBalance")
                req.amount = 0;
            else
                req.amount = (float)cur_request["amount"].GetDouble();

            req.status = 0;

            //11.20 set destBank and destAccount
            if(req.type == "transfer")
            {
                req.dest_bank_id = cur_request["destBank"].GetInt();
                req.dest_account_num = cur_request["destAccount"].GetInt();
                req.source_bank_id = bank_id;
            }
            else
            {
                req.dest_bank_id = -1;
                req.dest_account_num = 0;
                req.source_bank_id = -1;
            }
            req_que.push(req);
        }
        else if (cur_request.HasMember("seed")) //random request
        {
            int numReq = cur_request["numReq"].GetInt();
            int seed = cur_request["seed"].GetInt();
            srand(seed);
            string t1, t2;
            stringstream ss1;
            ss1 << bank_id;
            ss1 >> t1;
            stringstream ss2;
            ss2 << client_id;
            ss2 >> t2;
            //"req_range" : 20, "account_range" : 3, "amount_max":10000, "amount_dis":1
            int req_range = cur_request["req_range"].GetInt();
            int account_range = cur_request["account_range"].GetInt();
            int amount_range = cur_request["amount_range"].GetInt();
            int amount_dis = cur_request["amount_dis"].GetInt();

            int ac_base = 1000 * bank_id + 100 * client_id;
            /*get random request and put into queue*/
            for (unsigned int t = 0; t < numReq; ++t)
            {
                request_struct req;
                req.sent_id = sent_id;//11.8
                sent_id++;
                strcpy(req.client_ip, my_ip);
                req.client_port = my_port;
                /*get prob and upper bound*/
                float probGetBalance = (float)cur_request["probGetBalance"].GetDouble();
                float probDeposit = (float)cur_request["probDeposit"].GetDouble();
                float probWithdraw = (float)cur_request["probWithdraw"].GetDouble();
                float probTransfer = (float)cur_request["probTransfer"].GetDouble();
                float getBalance_bound = probGetBalance;//upper cound
                float deposit_bound = probGetBalance + probDeposit;
                float withdraw_bound = deposit_bound + probWithdraw;

                /*random req.type*/
                float type_prob =  (rand() % 100) * 0.01;
                if (type_prob >= 0 && type_prob < getBalance_bound)
                    req.type = "getBalance";
                else if (type_prob >= getBalance_bound && type_prob < deposit_bound)
                    req.type = "deposit";
                else if (type_prob >= deposit_bound && type_prob < withdraw_bound)
                    req.type = "withdraw";
                /*random account_num*/

                int ran_account_num =  rand() % account_range + ac_base + 1; //
                req.account_num = ran_account_num;
                /*random amount*/
                float ran_amount =  ((rand() % (amount_range / amount_dis)) + 1) * amount_dis; //
                req.amount = ran_amount;
                /*random req_id*/
                string ran_req_id;//bank_id.client_id.random_seq
                int ran_seq =  rand() % req_range + 1; //1-account_range
                // int to string
                stringstream ss3;
                string t3;
                ss3 << ran_seq;
                ss3 >> t3;
                ran_req_id = t1 + '.' + t2 + '.' + t3;
                req.req_id = ran_req_id;

                req.status = 0;
                req_que.push(req);
            }
        }
    }

    /*show client's info*/
    //show_info();
    //show_request();

    fclose(fp);
}
