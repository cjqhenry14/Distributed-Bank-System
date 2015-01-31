#include "common.h"

string Outcome[4]={"", "Processed", "InconsistentWithHistory", "InsufficientFunds"};

char * get_time()
{
    time_t timep;
    time(&timep);
    char *s = ctime(&timep);
    s[strlen(s)-1]='\0';
    return s;
}