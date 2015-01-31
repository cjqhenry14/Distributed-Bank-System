#include "process.h"

/*get balance*/
float get_account_balance(int account_num)
{
    if (balance_map.find(account_num) != balance_map.end())//exist
    {
        return balance_map[account_num];
    }
    else
    {
        float i = 0;
        balance_map[account_num] = i;
        return i;
    }
}
/*process query*/
int processQuery(request_struct req)
{
    if (balance_map.find(req.account_num) == balance_map.end())// not exist
    {
        balance_map[req.account_num] = 0;
    }

    return Processed;
}
/*check the request's occurence and the details about it*/
int check_same_req(request_struct req)
{
    /*check in the insufficentFundsTrans*/
    for (int k = 0; k < insuf_funds_trans.size(); ++k)//check in the insuf_funds_trans
    {
        if (req.req_id == insuf_funds_trans[k].req_id)//in insuf_funds_trans, type must be withdrae
        {
            if (req.type == insuf_funds_trans[k].type
                    && req.account_num == insuf_funds_trans[k].account_num
                    && req.amount == insuf_funds_trans[k].amount)//exactly the same request
            {
                if (req.type != "transfer")
                {
                    return EXACTLY_SAME_INSUF;
                }
                else if (req.type == "transfer" && req.dest_bank_id == insuf_funds_trans[k].dest_bank_id
                         && req.dest_account_num == insuf_funds_trans[k].dest_account_num)
                {
                    return EXACTLY_SAME_INSUF;
                }
            }

            return ONLY_SAME_REQID;
        }
    }

    /*check in the processedTrans*/
    for (int i = 0; i < processed_trans.size(); ++i)
    {
        if (req.req_id == processed_trans[i].req_id)
        {
            if (req.type == processed_trans[i].type &&
                    req.account_num == processed_trans[i].account_num
                    && req.amount == processed_trans[i].amount)//exactly the same request
            {
                if (req.type != "transfer")
                {
                    return EXACTLY_SAME_PROCESS;
                }
                else if (req.type == "transfer" && req.dest_bank_id == processed_trans[i].dest_bank_id
                         && req.dest_account_num == processed_trans[i].dest_account_num)
                {
                    return EXACTLY_SAME_PROCESS;
                }
            }

            return ONLY_SAME_REQID;
        }
    }
    return NOT_YET;
}
/*
if a transaction with this reqID
  has not been processed for this account, increase the account balance by
  the amount, append the transaction to processedTrans, and return
  <reqID, Processed, balance>, where balance is the current balance.  if
  exactly the same transaction has already been processed for this account,
  simply re-send the same reply.  if a different transaction
  with the same reqID has been processed for this account, return
  <reqID, InconsistentWithHistory, balance>.*/
int processDeposit(request_struct req)
{
    int acc_num;
    if(bank_id == req.dest_bank_id)//i am dest bank
        acc_num = req.dest_account_num;
    else 
        acc_num = req.account_num;//normal

    if (balance_map.find(acc_num) == balance_map.end())// not exist
    {
        balance_map[acc_num] = 0;
    }

    int check_res = check_same_req(req);
    if (check_res == NOT_YET)
    {
        balance_map[acc_num] += req.amount;

        fprintf(out, "(%-24s) UPDATE BALANCE:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f balance:%-8.2f\n",
                get_time(), req.type.c_str(), req.req_id.c_str(), acc_num, req.amount, balance_map[acc_num]);
        fflush(out);

        return Need;
    }
    else if (check_res == EXACTLY_SAME_PROCESS)
    {
        return Processed;
    }
    else //if (check_res == ONLY_SAME_REQID)
    {
        return InconsistentWithHistory;
    }
}
/*
  withdraw(reqId, accountNum, amount): if a transaction with this reqID
  has not been processed for this account, and the account balance is at
  least the amount, decrease the account balance by the amount, append the
  transaction to processedTrans, and return <reqID, Processed, balance>.
  if a transaction with this reqID has not been processed for this account,
  and the account balance is less than the amount, return <reqID,
  InsufficientFunds, balance>.  if exactly the same transaction has already
  been processed, simply re-send the same reply.  if a
  different transaction with the same reqID has been processed, return
  <reqID, InconsistentWithHistory, balance>.*/
int processWithdraw(request_struct req)
{
    if (balance_map.find(req.account_num) == balance_map.end())// not exist
    {
        balance_map[req.account_num] = 0;
    }

    int check_res = check_same_req(req);
    if (check_res == NOT_YET)
    {
        if (balance_map[req.account_num] >= req.amount)
        {
            balance_map[req.account_num] -= req.amount;

            fprintf(out, "(%-24s) UPDATE BALANCE:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f balance:%-8.2f\n",
                    get_time(), req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount, balance_map[req.account_num]);
            fflush(out);

            return Need;
        }
        else
        {
            return InsufficientFunds;
        }
    }
    else if (check_res == EXACTLY_SAME_PROCESS)
    {
        return Processed;
    }
    else if (check_res == EXACTLY_SAME_INSUF)
    {
        return InsufficientFunds;
    }
    else //if (check_res == ONLY_SAME_REQID)
    {
        return InconsistentWithHistory;
    }
}
int processTransfer(request_struct req)
{
    if (req.dest_bank_id != bank_id) // i am the source bank
    {
        return processWithdraw(req);
    }
    else// i am the dest bank
    {
        return processDeposit(req);
    }
}
/*update balance*/
void update_balance(request_struct req)
{
    //11.20
    if (req.type == "deposit")//i am dest, deposit
    {
        if (balance_map.find(req.account_num) == balance_map.end())// not exist
        {
            balance_map[req.account_num] = 0;
        }

        balance_map[req.account_num] += req.amount;

        fprintf(out, "(%-24s) UPDATE BALANCE:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f balance:%-8.2f\n",
                get_time(), req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount, balance_map[req.account_num]);
        fflush(out);
    }
    else if(req.type == "transfer" && bank_id == req.dest_bank_id)
    {
        if (balance_map.find(req.dest_account_num) == balance_map.end())// not exist
        {
            balance_map[req.dest_account_num] = 0;
        }

        balance_map[req.dest_account_num] += req.amount;
        fprintf(out, "(%-24s) UPDATE BALANCE:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f balance:%-8.2f\n",
                get_time(), req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount, balance_map[req.dest_account_num]);
        fflush(out);
    }
    else if ((req.type == "withdraw") ||
             (req.type == "transfer" && bank_id == req.source_bank_id))//i am source, withdraw
    {
        if (balance_map.find(req.account_num) == balance_map.end())// not exist
        {
            balance_map[req.account_num] = 0;
            printf("not enough money\n");
            return;
        }
        if (balance_map[req.account_num] < req.amount)
        {
            printf("not enough money\n");
            return;
        }
        balance_map[req.account_num] -= req.amount;
        fprintf(out, "(%-24s) UPDATE BALANCE:  type:%-10s reqID:%-8s accountNum:%-6d amount:%-8.2f balance:%-8.2f\n",
                get_time(), req.type.c_str(), req.req_id.c_str(), req.account_num, req.amount, balance_map[req.account_num]);
        fflush(out);
    }
}

/*delete request from sent*/
void delete_sent(request_struct req)
{
    for (list<request_struct>::iterator itor = sent.begin(); itor != sent.end();)
    {
        if (req.sent_id == itor->sent_id && req.client_port == itor->client_port)
        {
            /* delete from sent*/
            fprintf(out, "(%-24s) delete#%d from sent\n", get_time(), req.req_seq);
            fflush(out);
            itor = sent.erase(itor);
            break;
        }
        else
        {
            itor++;
        }
    }
}
/*check whether the request is in sent, if is resend, no need to store in sent.*/
bool check_in_sent(request_struct req)
{
    for (list<request_struct>::iterator itor = sent.begin(); itor != sent.end(); itor++)
    {
        if (req.sent_id == itor->sent_id && req.client_port == itor->client_port)
        {
            return true;
        }
    }
    return false;
}
int get_pre_send_status(request_struct req)
{
    for (list<request_struct>::iterator itor = sent.begin(); itor != sent.end(); itor++)
    {
        if (req.sent_id == itor->sent_id && req.client_port == itor->client_port)
        {
            return itor->status;
        }
    }
    return 0;//impossible
}

