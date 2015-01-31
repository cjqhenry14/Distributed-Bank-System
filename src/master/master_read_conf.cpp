#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filestream.h"

#include <cstdio>
#include <iostream>
#include <string>

#include "master.h"
#include "master_read_conf.h"

using namespace rapidjson;

void show_info()
{
    cout << "master_ip:" << master_ip << endl;
    cout << "master_port:" << master_port << endl;
    /*show each bank's server(ip, port)*/
    for (int i = 0; i < bank_vector.size(); ++i)
    {
        cout << "bank# " << i + 1 << "size: " << bank_vector[i].server_list.size() << endl;
        for (list<server_struct>::iterator it1 = bank_vector[i].server_list.begin(); it1 != bank_vector[i].server_list.end(); it1++)
        {
            cout << "server: " << it1->server_ip << ", " << it1->server_port << endl;
        }
        for (list<client_struct>::iterator it2 = bank_vector[i].client_list.begin(); it2 != bank_vector[i].client_list.end(); it2++)
        {
            cout << "client: " << it2->client_ip << ", " << it2->client_port << endl;
        }

    }
}
void get_master_need_info()
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

    /*get master info: ip, port*/
    unsigned int banks_num = bank.Size();
    const Value &master = bank[banks_num - 1]; //master info
    strcpy(master_ip, master["master_ip"].GetString());
    master_port = master["master_port"].GetInt();

    /*get each bank's server(ip, port) and client(ip, port)*/
    for (unsigned int i = 0; i < banks_num - 1; ++i)
    {
        bank_struct ban;
        //!!! now is in master accept server connect
        /*get server's info*/
       /* const Value &server_array = bank[i]["server"];
        for (unsigned int j = 0; j < server_array.Size(); ++j)
        {
            const Value &cur_server = server_array[j];
            if (cur_server["startup_delay"].GetInt() == 0)//and life time shouldn't be 0
            {
                server_struct ser;
                strcpy(ser.server_ip, cur_server["ip"].GetString());
                ser.server_port = cur_server["port"].GetInt();
                ban.server_list.push_back(ser);
            }
        }*/
        /*get client's info*/
        const Value &client_array = bank[i]["client"];
        for (unsigned int j = 0; j < client_array.Size(); ++j)
        {
            const Value &cur_client = client_array[j];

            client_struct cli;
            strcpy(cli.client_ip, cur_client["ip"].GetString());
            cli.client_port = cur_client["port"].GetInt();
            ban.client_list.push_back(cli);
        }

        bank_vector.push_back(ban);

    }


    /*show master's info*/
    //show_info();

    fclose(fp);
}
