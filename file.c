#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"
#include "data.h"
int readfile(char* path, char** destination, int* length)
{
	FILE* f = fopen(path, "r");
	if (f == NULL) return 1;
    if(fseek(f, 0, SEEK_END)) 
        if (fclose(f)) return -2;
        else return 2;
    long fsize = ftell(f);
    if (fsize == -1L)
        if (fclose(f)) return -3;
        else return 3;
    if(fseek(f, 0, SEEK_SET))
        if (fclose(f)) return -4;
        else return 4;
    *destination = malloc(fsize + 1);
    (*destination)[0] = 0;
    if (*destination == NULL)
        if (fclose(f)) return -5;
        else return 5;
    char* buf = malloc(sizeof(char)*(fsize+1));
    *length = 0;
    while (!feof(f))
    {
        if (fgets(buf, fsize, f))
        {
            strcat(*destination, buf);
            *length += strlen(buf);
        }
        if (ferror(f))
        {
            free(buf);
            if (fclose(f)) return 6;
            else return -6;
        }
    }
    free(buf);
    if (fclose(f)) return -1;
    return 0;
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

int getline(char* destination, char* source, int size, int* timesRead)
{
    if (*timesRead<0 || *timesRead > size / PufferSize) return -2;
    if(*timesRead < size / PufferSize)
    {
     // *destination=(char*)malloc((PufferSize+1)*sizeof(char));
     // if (*destination == NULL)
     //     return -1;
        strncpy(destination, source + *timesRead*PufferSize*sizeof(char), PufferSize);
        (*timesRead)++;
        return 0;
    }
 // *destination=(char*)malloc((1+size-*timesRead*PufferSize)*sizeof(char));
    strcpy(destination, source+*timesRead*PufferSize*sizeof(char));
    (*timesRead)++;
    return 1;
}
void addtolist(strlist* start, strlist* last, char* buf)
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
        last = next;
        start = next;
    }
    else
    {
        last->next = next;
        last = next;
    }
}