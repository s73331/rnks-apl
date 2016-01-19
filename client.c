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
#include <math.h>
#include "file.h"
#include "local.h"
#include "print.h"
#include "toUDP.h" //SAP to our protocol
#include "config.h"
#include "data.h"
#include "timer.h"
#include "sock.h"
#include "manipulation.h"
#pragma comment(lib, "Ws2_32.lib")	
#define BUFLEN 512
double errorQuota = -1;

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

/*
return:  
         0 - success
         1 - got last line
*/
int makeRequest(struct request* req, struct answer ans, strlist* strli, int toAnswer, unsigned long* lastSeNr, int* lastData)
{
    if (toAnswer)
    {
        if (ans.AnswType == AnswHello)
        {
            req->ReqType = ReqData;
            (*lastSeNr)++;
            req->SeNr = *lastSeNr;
            char buf[PufferSize + 1];
            buf[PufferSize] = 0;
            int gl;
            gl = getline(strli, req->SeNr - 1, buf); //-1, as SeNr=0 is not data
            if (gl < 0)
            {
                fprintf(stderr, "getting line failed with error code %i\nexiting...", gl);
                exit(6);
            }
            strncpy(req->name, buf, PufferSize);
            *lastSeNr = req->SeNr;
            return gl;
        }
        if (ans.AnswType == AnswNACK)
        {
            if (*lastData)                      // if we      have read the last line
            {
                if (*lastData == 1)             // if we JUST have read the last line
                {
                    (*lastSeNr)++;              // we increment lastSeNr the last time
                    (*lastData)++;              // and we remark, that we've done this
                }
                if (ans.SeNo == *lastSeNr)      // if server NACKs Close, send him Close
                {
                    req->ReqType = ReqClose;
                    req->SeNr = *lastSeNr;
                    return 0;
                }
            }
            req->SeNr = ans.SeNo;               // else send him datapacket
            if (req->SeNr > *lastSeNr) *lastSeNr = req->SeNr;
            req->ReqType = ReqData;
            char buf[PufferSize + 1];
            buf[PufferSize] = 0;
            int gl;
            gl = getline(strli, req->SeNr - 1, buf); //-1, as SeNo=0 is not data
            if (gl < 0)
            {
                fprintf(stderr, "getting line failed with error code %i\nexiting...", gl);
                exit(6);
            }
            strncpy(req->name, buf, PufferSize);
            return gl;
        }
    }
    else
    {
        if (req->SeNr == 0)
        {
            req->FlNr = 1;
            req->ReqType = ReqHello;
            return 0;
        }
        if (*lastData)                      // if we      have read the last line
        {
            if (*lastData == 1)             // if we JUST have read the last line
            {
                (*lastData)++;              // we increment lastSeNr the last time
                (*lastSeNr)++;              // and we remark, that we've done this
            }
            req->SeNr = *lastSeNr;          
            req->ReqType = ReqClose;
            return 0;
        }
        (*lastSeNr)++;
        req->SeNr = *lastSeNr;
        req->ReqType = ReqData;
        char buf[PufferSize + 1];
        buf[PufferSize] = 0;
        int gl;
        gl = getline(strli, req->SeNr - 1, buf); //-1, as SeNo=0 is not data
        if (gl < 0)
        {
            fprintf(stderr, "getting line failed with error code %i\nrequested line: %i\nexiting...", gl, req->SeNr - 1);
            exit(6);
        }
        strncpy(req->name, buf, PufferSize);
        return gl;
    }
    fprintf(stderr, "reached illegal position in makeRequest()\nexiting...");
    exit(1);
}
int sendRequest(struct request* req, struct answer ans, strlist* strli, int toAnswer, unsigned long* lastSeNr, int* lastData, SOCKET ConnSocket, struct timeouts** timeouts)
{
    int ret = makeRequest(req, ans, strli, toAnswer, lastSeNr, lastData);
    if (errorQuota >= 0 && (int)(errorQuota*RAND_MAX) > rand() || errorQuota<0 && NOSEND_ARRAY_SIZE > req->SeNr && NOSEND_DATA[req->SeNr])
        printReq(*req, 4);  // act like we sent the packet
    else
    {                       // actually send the packet
        int w = sendto(ConnSocket, (const char*)req, sizeof(*req), 0, resultMulticastAddress->ai_addr, resultMulticastAddress->ai_addrlen);
        if (w == SOCKET_ERROR) {
            fprintf(stderr, "send() failed: error %d\n", WSAGetLastError());
            exit(1);
        }
        printReq(*req, 1);
    }
    *timeouts=del_timer(*timeouts, req->SeNr, TRUE);
    *timeouts = add_timer(*timeouts, 1, req->SeNr+1);       // timer for next packet 
    if (req->ReqType != ReqHello)
        *timeouts = add_timer(*timeouts, TIMEOUT, req->SeNr);   // timer for window
    return ret;
}
int recvfromw(SOCKET ConnSocket, char* buf, size_t len, int flags, struct sockaddr* from, int* fromlen)
{
    int ret=recvfrom(ConnSocket, buf, len, flags, from, fromlen);
    if (ret == SOCKET_ERROR)
    {
        fprintf(stderr, "recvfrom() failed: error %d\n", WSAGetLastError());
        exit(1);
    }
    printAns(*((struct answer*)buf), 0);
    return ret;
}

int main(int argc, char *argv[])
{
    unsigned long window_size = 1;
    char* port = DEFAULT_PORT;
    char* server = DEFAULT_SERVER;
    char* filename = FILE_TO_READ;
    printf("Sender(Client)\n\n");
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if ((argv[i][0] == '-') || ((argv[i][0] == '/')) && (argv[i][1] != 0) && (argv[i][2] == 0))
            {
                switch (tolower(argv[i][1])) {
                case 'q':
                    if (argv[i + 1]){
                        if (argv[i + 1][0] != '-'){
                            errorQuota = strtod(argv[++i], NULL);
                            if (errorQuota == HUGE_VAL || errorQuota == -HUGE_VAL || errorQuota < 0) 
                                Usage(argv[0]);
                            break;
                        }
                    }
                    Usage(argv[0]);
                    break;
                case 'a':			//Server Address
                    if (argv[i + 1]) {
                        if (argv[i + 1][0] != '-') {
                            server = argv[++i];
                            break;
                        }
                    }
                    Usage(argv[0]);
                    break;
                case 'p':			//Server Port
                    if (argv[i + 1]) {
                        if (argv[i + 1][0] != '-') {
                            port = argv[++i];
                            break;
                        }
                    }
                    Usage(argv[0]);
                    break;
                case 'f':			//File Name
                    if (argv[i + 1]) {
                        if (argv[i + 1][0] != '-') {
                            filename = argv[++i];
                            if (strlen(filename) > 259 || strlen(filename) < 1) Usage(argv[0]);
                            break;
                        }
                    }
                    Usage(argv[0]);
                    break;
                case 'w':			//Window Size
                    if (argv[i + 1])
                    {
                        if (argv[i + 1][0] != '-') {
                            window_size = strtol(argv[++i], 0, 0);
                            if (window_size<1 || window_size>10) Usage(argv[0]);
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
            else Usage(argv[0]);
        }
    }
    struct timeouts* tl=NULL;                   // struct to store our timeouts
    struct answer ans;
    ans.SeNo = 0;
    struct request req;  
    req.SeNr = 0;
    fd_set fd;
    struct timeval tv;                          // struct for select-timevals
    tv.tv_sec = 0;
    unsigned long window_start =  0;
    strlist* strli=NULL;                        // struct for our file, which turned into a list of char-arrays (note: NOT strings, not even C-Strings)
    unsigned long lastSeNr = 0;                 // last sequence number we ever sent
    int lastData = 0;                           // 0 still have data, 1 have just read last line, 2 have read last line before
    int HelloAckRecvd = FALSE;
	initClient(server, port);
    if (readfilew(filename, &strli))
        fprintf(stderr, "closing file failed\ncontinuing...\n");
    lastData += sendRequest(&req, ans, strli, INITIAL, &lastSeNr, &lastData, ConnSocket, &tl);
    int stay = 1;
    while(stay)
	{
        if (!tl)                                //if we have nothing to wait for
        {
            if (lastSeNr < window_start + window_size)  // and the window allows us to send a packet
            {
                lastData += sendRequest(&req, ans, strli, INITIAL, &lastSeNr, &lastData, ConnSocket, &tl);
                continue;
            }
            continue;
        }
        SYSTEMTIME st;
        GetSystemTime(&st);
      //  printf("%i\t%d\t%d\t", window_start, lastSeNr, tl->seq_nr);
      //  printf("\t\t\t%02d.%03d\n", st.wSecond, st.wMilliseconds);
        if (req.ReqType != ReqClose)                 // the only time when we're truly waiting (NACK or CloseACK)
        {           
            
            fd_reset(&fd, ConnSocket);               // reset our fd for select
            tv.tv_usec = (tl->timer)*TO;             // get time of shortest timer
            int s = select(0, &fd, 0, 0, &tv);       // select if socket is read or timeout has passed
            if (!s) //timer expired                 
            {    
                if (tl->seq_nr > lastSeNr)          // if it is a packet we haven't sent
                {
                    if (lastSeNr < window_start + window_size)  // and the window allows us to send one
                    {
                        lastData += sendRequest(&req, ans, strli, INITIAL, &lastSeNr, &lastData, ConnSocket, &tl);
                        continue;
                    }
                    int seq = tl->seq_nr;
                    tl = del_timer(tl, tl->seq_nr, FALSE);
                    tl = add_timer(tl, 1, seq);
                    continue;
                }
                else if (window_start < tl->seq_nr || tl->seq_nr==1)
                {
                    window_start = tl->seq_nr;               // packet must've come through
                    tl = del_timer(tl, tl->seq_nr, FALSE);
                    continue;
                }
              //  tl = del_timer(tl, tl->seq_nr, FALSE);
               // printf("\t\t\t\t%d\n", tl->seq_nr);
               /* if (lastSeNr+1 < window_start + window_size)  // if the window allows us to send a packet
                {
                    lastData += sendRequest(&req, ans, strli, INITIAL, &lastSeNr, &lastData, ConnSocket, &tl);
                }*/
                continue;
            }
            if (s == SOCKET_ERROR)
            {
                fprintf(stderr, "select() failed: error %d\n", WSAGetLastError());
                exit(7);
            }
        }
        recvfromw(ConnSocket, (char*)&ans, sizeof(ans), 0, 0, 0);
        switch (ans.AnswType)
        {
            case AnswHello:
                if (HelloAckRecvd)
                {
                    fprintf(stderr, "Received another HelloACK\ncontinuing...\n");
                    continue;
                }
                tl = del_timer(tl, ans.SeNo, TRUE);
                lastData += sendRequest(&req, ans, strli, ANSWER, &lastSeNr, &lastData, ConnSocket, &tl);
                window_start++;
                HelloAckRecvd = 1;
                break;
            case AnswNACK:
                tl = del_timer(tl, ans.SeNo, TRUE);
                lastData += sendRequest(&req, ans, strli, ANSWER, &lastSeNr, &lastData, ConnSocket, &tl);
                continue;
            case AnswClose:
                stay = 0;
                continue;
            default:
                fprintf(stderr, "not recognized ans.AnswType\nexiting\n");
                exit(1);
        }
    }
	closesocket(ConnSocket);
	WSACleanup();
	return 0;
}
