#define _CRT_SECURE_NO_WARNINGS
#include "data.h"
#include <string.h>
#include <stdio.h>
void printAns(struct answer answ, int sent)
{
    printf("Answer\tNo: %i\ttype: %c\tstatus: ", answ.SeNo, answ.AnswType);
    if (sent) printf("S");
    else printf("R");
    printf("\n");
}
void printReq(struct request req, int flag)
{
    printf("Request\tNo: %i\ttype: %c\tstatus: ", req.SeNr, req.ReqType);
    switch (flag)
    {
    case 0: printf("R");
        break;
    case 1: printf("S");
        break;
    case 2: printf("C");
        break;
    case 3: printf("U");
        break;
    default:
        fprintf(stderr, "passed invalid flag to printReq(): %i", flag);
    }
    if (req.ReqType == ReqData)
    {
        char buf[PufferSize + 1];
        strncpy(buf, req.name, 10);
        buf[10] = 0;
        printf("\tdata: %s", buf);
    }
    printf("\n");
}