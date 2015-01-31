#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filestream.h"

#include "server.h"
#include "server_read_conf.h"

using namespace rapidjson;

/*get the
bank:(name, chain_length)
server:(ip, port, startup_delay, life_time)
pre_ip, pre_port
my_type
we have got [bank_id, server_id]
*/
void show_info()
{
    cout << "BANK # " << bank_id << endl;
    cout << "bank_name: " << bank_name << endl;

    cout << "MY IP: " << my_ip << " ";
    cout << "MY PORT: " << my_port << " ";

    cout << "PRE IP: " << pre_ip << " ";
    cout << "PRE PORT: " << pre_port << " ";

    cout << "startup_delay: " << startup_delay << " ";
    cout << "life_time: " << life_time << " ";
}
/* we have got [bank_id, server_id] */
void get_server_need_info()
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
    //11.20 get all banks' heads and tails
    for (unsigned int t = 0; t < banks_num - 1; ++t)
    {
        unsigned int h = 0;
        const Value &head_server = bank[t]["server"][h]["port"];
        all_heads.push_back(head_server.GetInt());
        //printf("all heads: %d\n", head_server.GetInt());

        unsigned int tnum = bank[t]["server"].Size() - 1;
        const Value &tail_server = bank[t]["server"][tnum]["port"];
        all_tails.push_back(tail_server.GetInt());
        //printf("%d\n", tail_server.GetInt());
    }

    const Value &master = bank[banks_num - 1]; //master info
    strcpy(master_ip, master["master_ip"].GetString());
    master_port = master["master_port"].GetInt();

    /*get bank basic info*/
    const Value &cur_bank = bank[bank_id - 1];
    bank_name = cur_bank["bank_name"].GetString();
    chain_length = cur_bank["chain_length"].GetInt();

    /*get server's self info (ip, port, startup_delay, life_time) */
    unsigned int server_i = server_id - 1;

    const Value &cur_server = cur_bank["server"][server_i];
    strcpy(my_ip, cur_server["ip"].GetString());
    my_port = cur_server["port"].GetInt();
    startup_delay = cur_server["startup_delay"].GetInt();
    life_time = cur_server["life_time"].GetString();
    //should wait for startup_delay and die until meet life_time
    if (life_time == "receive" || life_time == "send")
        msg_bound = cur_server["num"].GetInt();
    else
        msg_bound = 10000;

    /*head server*/
    if (server_id == 1 && chain_length > 1)
    {
        my_type = HEAD_TAIL;
        /*get next server's ip port*/
        const Value &next_server = cur_bank["server"][server_i + 1];
        strcpy(next_ip, next_server["ip"].GetString());
        next_port = next_server["port"].GetInt();
    }
    /*inter server*/
    if (server_id > 1 && server_id < chain_length)
    {
        my_type = TAIL;//11.11
        /*get pre server's ip, port*/
        const Value &pre_server = cur_bank["server"][server_i - 1];
        strcpy(pre_ip, pre_server["ip"].GetString());
        pre_port = pre_server["port"].GetInt();
        /*get next server's ip port*/
        const Value &next_server = cur_bank["server"][server_i + 1];
        strcpy(next_ip, next_server["ip"].GetString());
        next_port = next_server["port"].GetInt();
    }
    /*tail server*/
    if (server_id == chain_length && chain_length > 1)
    {
        my_type = TAIL;
        /*get pre server's ip, port*/
        const Value &pre_server = cur_bank["server"][server_i - 1];
        strcpy(pre_ip, pre_server["ip"].GetString());
        pre_port = pre_server["port"].GetInt();
    }
    /* head&tail server; only one server in chain*/
    if (chain_length == 1)
    {
        my_type = HEAD_TAIL;
    }

    /*show server's info*/
    //show_info();

    fclose(fp);
}
