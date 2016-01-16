#define _CRT_SECURE_NO_WARNINGS
#include "data.h"
#include <string.h>
#include <stdio.h>
void printAns(struct answer answ, int sent)
{
    printf("Answer\tNo: %i\ttype: %c\t", answ.SeNo, answ.AnswType);
    if (sent) printf("sent");
    else printf("received");
    printf("\n");
}
void printReq(struct request req, int sent)
{
    printf("Request\tNo: %i\ttype: %c\t", req.SeNr, req.ReqType);
    if (sent) printf("sent");
    else printf("received");
    if (req.ReqType == ReqData)
    {
        char buf[PufferSize + 1];
        strncpy(buf, req.name, 10);
        buf[10] = 0;
        printf("\tdata: %s", buf);
    }
    printf("\n");
}