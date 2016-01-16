#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int readfile(char* path, char** destination)
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
    buf[0] = 0;
    while (!feof(f))
    {
        if(fgets(buf, fsize, f))
            strcat(*destination, buf);
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

int writefile(char* path, char* string)
{
    FILE* f = fopen(path, "w");
    if (f == NULL) return 1;
    if(fputs(string, f)<0) 
        if (fclose(f)) return -2;
        else return 2;
    if(fclose(f)) return -1;
    return 0;
}