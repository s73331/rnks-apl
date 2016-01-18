#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cache.h"
#include "data.h"
void insert(struct _cache** start, struct request* newelem)
{
    struct _cache* newel = (struct _cache*)malloc(sizeof(struct _cache));
    if (!newel)
    {
        fprintf(stderr, "error when mallocing\nexiting");
        exit(1);
    }
    newel->next = (void*)0;
    newel->req.SeNr = newelem->SeNr;
    newel->req.FlNr = newelem->FlNr;
    newel->req.ReqType = newelem->ReqType;
    strncpy(newel->req.name, newelem->name, PufferSize);
    if (!(*start))
    {
        *start = newel;
        return;
    }
    struct _cache* help=*start;
    while (help->next && help->req.SeNr < newelem->SeNr) help = help->next;
        //iterate through list, until either list ends or SeNr is greater or equal
    if (help->req.SeNr == newelem->SeNr) return;
    newel->next = help->next;
    help->next = newel;
    return;
}
int peek(struct _cache* start)
{
    if (!start) return 0;
    return start->req.SeNr; //lowest SeNr is always at start
}
struct _cache* get(struct _cache** start)
{
    if (!(*start)) return (void*)0;
    struct _cache* help = *start;
    *start = (*start)->next;
    return help;
}