#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"
#include "data.h"
int readfile(char* path, struct _strlist** start)
{
	FILE* f = fopen(path, "r");
	if (f == NULL) return 1;
    if(fseek(f, 0, SEEK_END))           // go to the end of the file
        if (fclose(f)) return -2;
        else return 2;
    long fsize = ftell(f);              // get the position = number of chars
    if (fsize == -1L)
        if (fclose(f)) return -3;
        else return 3;
    if(fseek(f, 0, SEEK_SET))           // back to start
        if (fclose(f)) return -4;
        else return 4;
    char *temp = malloc((fsize + 1)*sizeof(char));
    if (temp == NULL)
        if (fclose(f)) return -5;
        else return 5;
    (*temp) = 0;
    char* buf = malloc(sizeof(char)*(fsize + 1));
    (*buf) = 0;
    int length = 0;
    while (!feof(f))                    // while file has not ended
    {
        if (fgets(buf, fsize, f))       // get a line or fsize chars
        {
            buf[fsize + 1] = 0;         // get the string an end, if fsize chars were read
            strcat(temp, buf);          // and cat that to temp
            length += strlen(buf);
        }
        if (ferror(f))
        {
            free(buf);
            free(temp);
            if (fclose(f)) return 6;
            else return -6;
        }
    }
    strlist* li;
    strlist* help;
    li = malloc(sizeof(strlist));
    if (li==NULL) return 5;
    li->next = NULL;
    *start = li;
    strncpy(li->str, temp, PufferSize);           // copy first part into li->str
    for (int i = 1; i*PufferSize < length; i++)
    {
        help = malloc(sizeof(strlist));
        if (!help) return 5;
        li->next = help;
        li = li->next;
        li->next = NULL;
        strncpy(li->str, temp + i*PufferSize*sizeof(char), PufferSize);
                                                  // copy next part into li->str
    }
    if (fclose(f)) return -1;
    return 0;
}
int readfilew(char* path, struct _strlist** start)
{
    int ret = readfile(path, start);
    if (!ret || ret == -1) return ret;
    fprintf(stderr, "readfile() returned code %i\nexiting", ret);
    exit(1);
}
int writefile(char* path, strlist* start)
{
    FILE* f = fopen(path, "w");
    if (f == NULL) return 1;
    char buf[PufferSize + 1];
    while (start != NULL)
    {
        strncpy(buf, start->str, PufferSize);
        buf[PufferSize] = 0;
        if (fputs(buf, f) < 0)
            if (fclose(f)) return -2;
            else return 2;
        start = start->next;
    }
    if (fclose(f)) return -1;
    return 0;
}
int getline(strlist* strl, int identifier, char* destination)
{
    if (!strl||!destination) return -1;     // either NULL
    struct _strlist* help = strl;
    for (; identifier; identifier--)
    {
        if (!help->next) return -2;         // illegal identifier
        help = help->next;
    }
    strncpy(destination, help->str, PufferSize);
    if (help->next) return 0;               // if we have a next line return 0
    return 1;                               // if not 1
}
struct _strlist* addtolist(strlist* start, char* buf)
{
    strlist* next = (strlist*)malloc(sizeof(strlist));
    if (next == NULL)
    {
        fprintf(stderr, "error when mallocing\nexiting...");
        exit(1);
    }
    next->next = NULL;
    strncpy(next->str, buf, PufferSize);
    if (start == NULL)
    {
        start = next;
        return start;
    }
    strlist* help=start;
    while(help->next!=NULL)
    {
        help = help->next;
    }
    help->next = next;
    return start;
}