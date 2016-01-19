#define _CRT_SECURE_NO_WARNINGS
#include "data.h"
#include <string.h>
#include <stdio.h>
#include <Windows.h>
void printAns(struct answer answ, int sent)
{
    printf("Answer\tNo: %i\ttype: %c\tstatus: ", answ.SeNo, answ.AnswType);
    if (sent) printf("Sent\t");
    else printf("Received");
    SYSTEMTIME st;
    GetSystemTime(&st);
    printf("\t%02d.%03d\n", st.wSecond, st.wMilliseconds);
}
void printReq(struct request req, int flag)
{
    printf("Request\tNo: %i\ttype: %c\tstatus: ", req.SeNr, req.ReqType);
    switch (flag)
    {
    case 0: printf("Received");
        break;
    case 1: printf("Sent\t");
        break;
    case 2: printf("Cached\t");
        break;
    case 3: printf("Uncached");
        break;
    case 4: printf("Manipulating");
        break;
    case 5: printf("Ignoring");
        break;
    default:
        fprintf(stderr, "passed invalid flag to printReq(): %i", flag);
    }
    SYSTEMTIME st;
    GetSystemTime(&st);
    printf("\t%02d.%03d", st.wSecond, st.wMilliseconds);
    if (req.ReqType == ReqData)
    {
        char buf[PufferSize + 1];
        strncpy(buf, req.name, 10);
        buf[10] = 0;                    // got no more space
        printf("\tdata: %s", buf);
    }
    printf("\n");
}