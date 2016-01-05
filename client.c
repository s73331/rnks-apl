//only some parts...of the clientApp
#include "toUdp.h" // SAP to our protocol
#include "config.h"
#include <dos.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <winsock2.h>
#include "data.h"

void Usage(char *ProgName) // How to use program
{
    fprintf(stderr, P_MESSAGE_1);
    fprintf(stderr, P_MESSAGE_6, ProgName);
    fprintf(stderr, P_MESSAGE_7, (DEFAULT_SERVER == NULL) ? "loopback address" : DEFAULT_SERVER);
    fprintf(stderr, P_MESSAGE_8, DEFAULT_PORT);
    fprintf(stderr, P_MESSAGE_9);
    exit(1);
}
int printAnswer(struct answer *answPtr)
{
    switch(answPtr->AnswType) {
        case AnswHello:
            printf("Answer Hello");
            break;
        case AnswOk:
            printf("Answer 0k : Packet acknowledged No: %u ", answPtr->SeNo);
            break;
        case AnswNACK:
            printf("Answer NACK : Packet negative acknowledged No: %u ", answPtr->SeNo);
            break;
        case AnswClose :
            printf("Answer Close");
            break;
        case AnswErr :
            printf("Answer Error: %s",errorTable[answPtr->ErrNo]);
            break;
        default:
            printf("Illegal Answer");
            break;
        }; /* switch */
        puts("\n");
        return answPtr->AnswType;
} /* printAnswer */
int main(int argc, char *argv[])
{
	//...
	long i;
	char *Server = DEFAULT_SERVER;
	char *Filename = "";
	char *Port = DEFAULT_PORT;
	char *Window_size = DEFAULT_WINDOW;

	FILE *fp;

	//Parameter überprüfen
	if (argc > 1){
		for (i = 1; i < argc; i++) {
			if (((argv[i][0] == '-') || (argv[i][0] == '/')) && (argv[i][1] != 0) && (argv[i][2] == 0)) {
				switch (tolower(argv[i][1])) {
				case 'a': //Server Address
					if (argv[i + 1]) {
						if (argv[i + 1][0] != '-') {
							Server = argv[++i];
							break;
						}
					}
					Usage(argv[0]);
					break;
				case 'p': //Server Port
					if (argv[i + 1]){
						if (argv[i + 1][0] != '-') {
							Port = argv[++i];
							break;
						}
					}
					Usage(argv[0]);
					break;
				case 'f': //File Name
					if (argv[i + 1]) {
						if (argv[i + 1][0] != '-') {
							Filename = argv[++i];
						}
					}
					Usage(argv[0]);
					break;
				case 'w': //Window size
					if (argv[i + 1]) {
						if (argv[i + 1][0] != '-') {
							Window_size = argv[++i];
							break;
						}
					}
					Usage(argv[0]);
					break;
				default:
					Usage(argv[0]);
					break;
				}
			}
			else
				Usage(argv[0]);
		}
	}
	//...
}
