#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <dos.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <Winsock2.h>
#include <errno.h>
#include <ctype.h>
#include <tchar.h>
#include <ws2tcpip.h>
#include "file.h"
#include "local.h"
#include "print.h"
#include "toUDP.h" //SAP to our protocol
#include "config.h"
#include "data.h"
#include "timer.h"
#include "sock.h"
#pragma comment(lib, "Ws2_32.lib")	

#define BUFLEN 512

struct request req;

/****************************************************/
/*** Declaration socket descriptor "s"          *****/
/****************************************************/
SOCKET ConnSocket;

/****************************************************/
/***Declaration of socket addresss "local" static ***/
/****************************************************/
static struct sockaddr_in6 localAddr;
struct sockaddr *sockaddr_ip6_local = NULL;

/****************************************************/
/***Declaration of socket address "remote" static ***/
/****************************************************/
static struct addrinfo *resultMulticastAddress = NULL;


void Usage(char *ProgName)					// How to use program
{
	fprintf(stderr, P_Message_1);
	fprintf(stderr, P_Message_6, ProgName);
	fprintf(stderr, P_Message_7, (DEFAULT_SERVER == NULL) ? "loopback address" : DEFAULT_SERVER);
	fprintf(stderr, P_Message_8, DEFAULT_PORT);
	fprintf(stderr, P_Message_9);
	fflush(stdin);
	getchar();
	exit(1);
}

int printAnswer(struct answer *answPtr) {
	switch (answPtr->AnswType) {
	case AnswHello:
		printf("answer Hello");
		break;
	case AnswOk:
		printf("answer Ok : Packet acknowledged No: %u ", answPtr->SeNo);
		break;
	case AnswNACK:
		printf("Answer NACK : Packet negative acknowledged No: %u ", answPtr->SeNo);
		break;
	case AnswClose:
		printf("Answer Close");
		break;
	case AnswErr:
		printf("Answer Error: %s", errorTable[answPtr->ErrNo]);
		break;
	default:
		printf("Illegal Answer");
		break;
	};
	puts("\n");
	return answPtr->AnswType;
}

/*int main(int argc, char *argv[])
{
//...
char fileText[99][99];
long i;

char *Server = DEFAULT_SERVER;
char *Filename = "";
char *Port = DEFAULT_PORT;
char *window_size = DEFAULT_WINDOW;				//TODO: make a real fix

FILE *fp;										//TODO: pointer auf file das gesendet werden soll

//Parameter ueberpruefen
if (argc > 1) {

for (i = 1;i < argc;i++) {
//printf("1\n");
if (((argv[i][0] == '0') || (argv[i][0] == '-')) && (argv[i][1] != 0) && (argv[i][2] == 0)) {
switch (tolower(argv[i][1])) {
case 'a':			//Server Address
if (argv[i + 1]) {
if (argv[i + 1][0] != '-') {
Server = argv[++i];
break;
}
}
Usage(argv[0]);
break;
case 'p':			//Server Port
if (argv[i + 1]) {
if (argv[i + 1][0] != '-') {
Port = argv[++i];
//printf("2\n");
break;
}
}
Usage(argv[0]);
break;
case 'f':			//File Name
if (argv[i + 1]) {
if (argv[i + 1][0] != '-') {
Filename = argv[++i];
break;
}
}
Usage(argv[0]);
break;
case 'w':			//window size
if (argv[i + 1]) {
if (argv[i + 1][0] != '-') {
window_size = argv[++i];
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
else {
Usage(argv[0]);
}

printf("Inhalt der Datei:\t%s\n", Filename);													//fix first
//open file

fp = fopen(Filename, "r");
char inhalt[2] = " ";
char test = ' ';
int g = 0;
if (fp != NULL) {
while (feof(fp)==NULL) {

//fgets(inhalt, 2, fp);

//printf("%c\n",inhalt[0]);

printf("%c",fgetc(fp));
;
}
printf("\nfertig");
}
else {
printf("datei kann nicht gelesen werden \n");

}


fclose(fp);

fflush(stdin);
getchar();

//...
}*/

int initClient(char *MCAddress, char *Port) {


	int trueValue = 0, loopback = 1; //setsockopt
	int val, i = 0;
	int addr_len;
	struct ipv6_mreq mreq; //multicast address
	struct addrinfo *resultLocalAddress = NULL, *ptr = NULL, hints;

	WSADATA wsaData;
	WORD wVersionRequested;

	wVersionRequested = MAKEWORD(2, 1);
	if (WSAStartup(wVersionRequested, &wsaData) == SOCKET_ERROR) {
		fprintf(stderr, "SERVER: WSAStartup() failed\n");
		fprintf(stderr, "        error code: %d\n", WSAGetLastError());
		exit(-1);
	}

	/****************************************************/
	/***Create Socket, 				  ***/
	/***connectionless service, addresss family INET6 ***/
	/****************************************************/
	ConnSocket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

	if (ConnSocket <0) {
		fprintf(stderr, "Client: Error Opening socket: Error %d\n", WSAGetLastError());
		WSACleanup();
		exit(-1);
	}

	/* Initialize socket */
	/* Reusing port for several server listening on same multicast addr and port
	(if we are testing on local machine only) */
	setsockopt(ConnSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&trueValue, sizeof(trueValue));

	/* Resolve multicast group address to join mc group */

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_flags = AI_NUMERICHOST;
	if (getaddrinfo(MCAddress, Port, &hints, &resultMulticastAddress) != 0) {					//TODO: sendet nur an DEFAULT_PORT
		fprintf(stderr, "getaddrinfo MCAddress fauled with error: %d\n", WSAGetLastError());
		WSACleanup();
		exit(-1);
	}


	/* Resolve local address (anyaddr) to bind*/
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	hints.ai_flags = AI_PASSIVE; // localhost

	val = getaddrinfo(NULL, Port, &hints, &resultLocalAddress);

	if (val != 0) {
		fprintf(stderr, "getaddrinfo localAddress failed with error: %d\n", val);
		WSACleanup();
		exit(-1);
	}

	//Retrive the address and print  out the hex bytes
	for (ptr = resultLocalAddress; ptr != NULL; ptr = ptr->ai_next) {
		printf("getaddrinfo response %d\n", i++);
		printf("\tFlags: 0x%x\n", ptr->ai_flags);
		printf("\tFamilty: ");
		switch (ptr->ai_family) {

		case AF_UNSPEC:
			printf("Unspecified\n");
			break;
		case AF_INET:
			printf("AF_INET (IPv4)\n");
			break;
		case AF_INET6:
			printf("AF_INET6 (IPv6)\n");

			sockaddr_ip6_local = (struct sockaddr *) ptr->ai_addr;
			addr_len = ptr->ai_addrlen;
			break;
		default:
			printf("Other %d\n", ptr->ai_family);
			break;
		}

	}

	/****************************************************/
	/*** Bind Socket ***/
	/****************************************************/
	printf("in bind\n");

	/*if (bind(ConnSocket, sockaddr_ip6_local, addr_len) == SOCKET_ERROR) {
	fprintf(stderr, "bind() failed: error %d\n", WSAGetLastError());
	WSACleanup();
	fflush(stdin);
	getchar();
	exit(-1);
	}/*


	/****************************************************/
	/* Specify the multicast group                      */
	/****************************************************/
	memcpy(&mreq.ipv6mr_multiaddr, &((struct sockaddr_in6*)(resultMulticastAddress->ai_addr))->sin6_addr, sizeof(mreq.ipv6mr_multiaddr));

	/* Accept multicast from any interface */
	// scope ID from Int. -> to get scopeid :netsh int ipv6 sh addr or ipconfig -all
	mreq.ipv6mr_interface = INADDR_ANY; //my w8 Laptop

	/*//Join the multicast address (netsh interface ipv6 show joins x)
	if (setsockopt(ConnSocket, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0) {
	fprintf(stderr, "setsockopt(IPV6_JOIN_GROUP) failed %d\n", WSAGetLastError());
	WSACleanup();
	fflush(stdin);
	getchar();
	exit(-1);
	}
	*/

	freeaddrinfo(resultLocalAddress);
	//freeaddrinfo(resultMulticastAddress);

	return(0);
}



int main(int argc, char *argv[])
{
    printf("Sender(Client)\n\n");
    int stay = 1;
    int stay2 = 1;
    struct timeouts* tl=NULL;
    struct answer ans;
    int w;
    struct request req;
    fd_set fd;
    struct timeval tv;
    tv.tv_sec = 0;
    
	initClient(DEFAULT_SERVER, DEFAULT_PORT);
    req.FlNr = 1;
    req.ReqType = ReqHello;
    req.SeNr = 0;
    while (stay)
    {
        w = sendto(ConnSocket, (const char*)&req, sizeof(req), 0, resultMulticastAddress->ai_addr, resultMulticastAddress->ai_addrlen);
        if (w == SOCKET_ERROR) {
            fprintf(stderr, "send() failed: error %d\n", WSAGetLastError());
            exit(1);
        }
        printReq(req, 1);
        tl = add_timer(tl, 1, req.SeNr);
        tv.tv_usec = (tl->timer)*TO;
        fd_reset(&fd, ConnSocket);
        int s=select(0, &fd, 0, 0, &tv);
        if (!s) //timer expired
        {
            decrement_timer(tl);
            tl=del_timer(tl, req.SeNr);
            continue;
        }
        if(s==SOCKET_ERROR)
        {
            fprintf(stderr, "select() failed: error %d\n", WSAGetLastError());
            exit(2);
        }
        stay = 0;
        tl = del_timer(tl, req.SeNr);
    }
    int recvcc = recvfrom(ConnSocket, (char*)&ans, sizeof(ans), 0, 0, 0);
    if (recvcc == SOCKET_ERROR)
    {
        fprintf(stderr, "recvfrom() failed: error %d\n", WSAGetLastError());
        exit(3);
    }
    printAns(ans, 0);
    if (ans.AnswType != AnswHello)
    {
        fprintf(stderr, "ans.answType not AnswHello: %c\nexiting...", ans.AnswType);
        exit(4);
    }
    strlist* strli = NULL;
    int r;
    if (r=readfile(FILE_TO_READ, &strli))
    {
        fprintf(stderr, "reading file failed with error code: %i\nexiting...", r);
        exit(5);
    }
    stay = 1;
    while(stay)
	{
        req.ReqType = ReqData;
        req.SeNr++;
        if (req.SeNr == 2) req.SeNr = 4;
        char buf[PufferSize+1];
        buf[PufferSize] = 0;
        int gl;
        gl = getline(strli, req.SeNr - 1, buf); //-1, as SeNo=0 is not data
        if (gl > 0) stay = 0;
        if (gl < 0)
        {
            fprintf(stderr, "getting line failed with error code %i\nexiting...",gl);
            exit(6);
        }
        strncpy(req.name, buf, PufferSize);
        stay2 = 1;
        while (stay2)
        {
            fd_reset(&fd, ConnSocket);
            w = sendto(ConnSocket, (const char*)&req, sizeof(req), 0, resultMulticastAddress->ai_addr, resultMulticastAddress->ai_addrlen);
            if (w == SOCKET_ERROR) {
                fprintf(stderr, "send() failed: error %d\n", WSAGetLastError());
                getchar();
                exit(6);
            }
            else printReq(req, 1);
            fd_reset(&fd, ConnSocket);
            tl = add_timer(tl, 1, req.SeNr);
            tv.tv_usec = (tl->timer)*TO;
            int s = select(0, &fd, 0, 0, &tv);
            if (!s) //timer expired
            {
                decrement_timer(tl);
                tl=del_timer(tl, req.SeNr);
                stay2 = 0;
                continue;
            }
            if (s == SOCKET_ERROR)
            {
                fprintf(stderr, "select() failed: error %d\n", WSAGetLastError());
                exit(7);
            }
            tl=del_timer(tl, req.SeNr);
            recvcc = recvfrom(ConnSocket, (char*)&ans, sizeof(ans), 0, 0, 0);
            if (recvcc == SOCKET_ERROR)
            {
                fprintf(stderr, "recvfrom() failed: error %d\n", WSAGetLastError());
                exit(8);
            }
            printAns(ans, 0);
            if (ans.AnswType != AnswNACK)
            {
                fprintf(stderr, "ans.answType not AnswNACK: %c\nexiting...", ans.AnswType);
                exit(9);
            }
            req.ReqType = ReqData;
            gl = getline(strli, ans.SeNo - 1, buf); //-1, as SeNo=0 is not data
            if (gl > 0) stay = 0;
            if (gl < 0)
            {
                fprintf(stderr, "getting line failed with error code %i\nexiting...", gl);
                exit(6);
            }
            strncpy(req.name, buf, PufferSize);
            int seqnr = req.SeNr;
            req.SeNr = ans.SeNo;
            w = sendto(ConnSocket, (const char*)&req, sizeof(req), 0, resultMulticastAddress->ai_addr, resultMulticastAddress->ai_addrlen);
            if (w == SOCKET_ERROR) {
                fprintf(stderr, "send() failed: error %d\n", WSAGetLastError());
                exit(5);
            }
            req.SeNr = seqnr;
        }
	}
    req.ReqType = ReqClose;
    req.SeNr++;
    stay = 1;
    while (stay)
    {
        w = sendto(ConnSocket, (const char*)&req, sizeof(req), 0, resultMulticastAddress->ai_addr, resultMulticastAddress->ai_addrlen);
        if (w == SOCKET_ERROR) {
            fprintf(stderr, "send() failed: error %d\n", WSAGetLastError());
        }
        fd_reset(&fd, ConnSocket);
        printReq(req, 1);
        tl = add_timer(tl, 1, req.SeNr);
        tv.tv_usec = (tl->timer)*TO;
        int s = select(0, &fd, 0, 0, &tv);
        if (!s) //timer expired
        {
            decrement_timer(tl);
            tl = del_timer(tl, req.SeNr);
            continue;
        }
        if (s == SOCKET_ERROR)
        {
            fprintf(stderr, "select() failed: error %d\n", WSAGetLastError());
            exit(10);
        }
        tl = del_timer(tl, req.SeNr);
        stay = 0;
    }
    recvcc = recvfrom(ConnSocket, (char*)&ans, sizeof(ans), 0, 0, 0);
    if (recvcc == SOCKET_ERROR)
    {
        fprintf(stderr, "recvfrom() failed: error %d\n", WSAGetLastError());
        exit(11);
    }
    printAns(ans, 0);
    if (ans.AnswType != AnswClose)
    {
        fprintf(stderr, "ans.answType not AnswClose: %c\nexiting...", ans.AnswType);
        exit(12);
    }
	closesocket(ConnSocket);
	WSACleanup();

	return 0;
}
