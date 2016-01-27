/* Win32 Konsole Version -  connectionless */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "data.h"
#include "config.h"
#include "toUdp.h"
#include "print.h"
#include "file.h"
#include "local.h"
#include "manipulation.h"
#include "cache.h"
#include "sock.h"
#include "timer.h"
double errorQuota = 0;
int manipulating = 0;
int DEBUG_TIMER = 0;

#pragma comment(lib, "Ws2_32.lib")				// necessary for the WinSock2 lib

#define BUFLEN 512

/****************************************************/
/*** Declaration socket descriptor "s"          *****/
/****************************************************/
SOCKET ConnSocket;

/****************************************************/
/***Declaration of socket addresss "local" static ***/
/****************************************************/
//static struct sockaddr_in6 localAddr;
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

int initServer(char *MCAddress, char *Port) {
	int trueValue = 1, loopback = 1; //setsockopt
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
	/*** Create Socket, 				  ***/
	/*** connectionless service, addresss family INET6 ***/
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
	setsockopt(ConnSocket, LEVEL, OPTNAME, (char *)&trueValue, sizeof(trueValue));

	/* Resolve multicast group address to join mc group */

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_flags = AI_NUMERICHOST;
	if (getaddrinfo(MCAddress, Port, &hints, &resultMulticastAddress) != 0) {
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

	//Retrieve the address and print  out the hex bytes
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

	if (bind(ConnSocket, sockaddr_ip6_local, addr_len) == SOCKET_ERROR) {
		fprintf(stderr, "bind() failed: error %d\n", WSAGetLastError());
		WSACleanup();
		fflush(stdin);
		getchar();
		exit(-1);
	}


	/****************************************************/
	/* Specify the multicast group                      */
	/****************************************************/
	memcpy(&mreq.ipv6mr_multiaddr, &((struct sockaddr_in6*)(resultMulticastAddress->ai_addr))->sin6_addr, sizeof(mreq.ipv6mr_multiaddr));


	/* Accept multicast from any interface */
	// scope ID from Int. -> to get scopeid :netsh int ipv6 sh addr or ipconfig -all
	mreq.ipv6mr_interface = IPV6MR_INTERFACE; 

	/*Join the multicast address (netsh interface ipv6 show joins x)*/
	if (setsockopt(ConnSocket, IPPROTO_IPV6, IPV6_JOIN_GROUP, (const char*)&mreq, sizeof(mreq)) < 0) {
		fprintf(stderr, "setsockopt(IPV6_JOIN_GROUP) failed %d\n", WSAGetLastError());
		WSACleanup();
		fflush(stdin);
		getchar();
		exit(-1);
	}

	//freeaddrinfo(resultLocalAddress);
	//freeaddrinfo(resultMulticastAddress);

	return(0);
}

void getRequest(struct request* req) {
	int recvcc;		     /*Length of received Message */
	int remoteAddrSize = sizeof(struct sockaddr_in6);
    struct request reqfake;
    if (manipulating) recvcc = recvfrom(ConnSocket, (char*)&reqfake, sizeof(struct request), 0, (struct sockaddr *) resultMulticastAddress, &remoteAddrSize);
    else
        /* Receive a message from a socket */
        recvcc = recvfrom(ConnSocket, (char*)req, sizeof(struct request), 0, (struct sockaddr *) resultMulticastAddress, &remoteAddrSize);
    if (recvcc == SOCKET_ERROR) {
        fprintf(stderr, "recv() failed: error %d\n", WSAGetLastError());
        closesocket(ConnSocket);
        WSACleanup();
        exit(-3);
    }
    if (recvcc == 0) {
        printf("Client closed connection\n");
        closesocket(ConnSocket);
        WSACleanup();
        exit(-6);
    }
    if (manipulating)
        printReq(reqfake, 4);
    else printReq(*req, 0);
	return;
}

void sendAnswer(struct answer * answ) {
	/****************************************************/
	/*** Send NACK back to Unicast Address            ***/
	/****************************************************/
	int w;
    	w = sendto(ConnSocket, (const char *)answ, sizeof(*answ), 0, (struct sockaddr *)resultMulticastAddress, sizeof(struct sockaddr_in6));
	if (w == SOCKET_ERROR)
		fprintf(stderr, "send() failed: error %d\n", WSAGetLastError());
    	else printAns(*answ, 1);
}

int exitServer() {
	/********************/
	/*** Close Socket ***/
	/********************/
	closesocket(ConnSocket);
	printf("in exit server\n");

	if (WSACleanup() == SOCKET_ERROR) {
		fprintf(stderr, "Server: WSACleanup() failed!\n");
		fprintf(stderr, "        error code: %d\n", WSAGetLastError());
		exit(-4);
	}
	return(0);
}

void answreturn(struct answer* answ, struct request *reqPtr, unsigned int expectedSequence)
{
     //struct answer* answ = malloc(sizeof(answ)); brings up heap corruption when writing to answ->SeNo, gonna play safe
    switch (reqPtr->ReqType)
	{
	case ReqHello:
		answ->AnswType = AnswHello;
		answ->SeNo = reqPtr->SeNr;
        reqPtr->ReqType = ReqData; // we answered HelloACK, next time we will NACK if no more packet comes until timeout
		return;
		break;
    case ReqClose:
        if (expectedSequence < reqPtr->SeNr)
        {
            answ->AnswType = AnswNACK;
            answ->SeNo = expectedSequence;
            return;
        }
        answ->AnswType = AnswClose;
        answ->SeNo = reqPtr->SeNr;
        return;
        break;
    case ReqData:
        answ->AnswType = AnswNACK;
        answ->SeNo = expectedSequence;
        return;
        break;
    default:
		printf("couldn't generate answer\nexiting\n");
        exit(1);
		break;
	}
}


int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));
    int i = 0;
    char *server = DEFAULT_SERVER;
    char *filename = FILE_TO_WRITE;
    char* port = DEFAULT_PORT;
    int window_size = DEFAULT_WINDOW;
    //Parameter check
    if (argc > 1) {
        for (i = 1; i < argc; i++) {
            if ((argv[i][0] == '-')||((argv[i][0] == '/'))&&(argv[i][1] != 0)&&(argv[i][2] == 0))
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
                default:
                    Usage(argv[0]);
                    break;
                }
            }
            else Usage(argv[0]);
        }
    }
    initServer(server, port);
    struct answer ans;
    unsigned long expectedSequence = 0;
    struct timeouts* tl = NULL;         //struct to manage timeouts
    strlist* strl = NULL;               //to store strings of packets in order
    cache* rc = NULL;                   //to store packets not in order
    int stay = 1;
    int s;
    struct request req;
    struct timeval tv;                  //struct for select, if timeout has passed
    tv.tv_sec = 0;
	int window_start = 0;
	int markedWindow[] = { 0 };
    fd_set fd;                          //struct for select, if one of the sockets is ready for work
    getRequest(&req);
    if (req.ReqType != ReqHello)
    {
        fprintf(stderr, "expected ReqHello\nexiting...");
        exit(1);
    }
    answreturn(&ans, &req, expectedSequence);
    sendAnswer(&ans);
    expectedSequence++;
	window_size = req.FlNr;
	SYSTEMTIME st1, st2;
	int timerNumber = 1;
	tl = add_timer(tl, TIMEOUT_MULTI, timerNumber);
	while (stay)
    {   
		fd_reset(&fd, ConnSocket);                  // add ConnSocket to struct, needs to be redone before every select
		if (tl && tl->timer)
	        tv.tv_usec = (long)(INT_MS+15000);			// add 150millis as otherwise our NACK would cross their datapacket
		else tv.tv_usec = 0L;
		GetSystemTime(&st1);
		s = select(0, &fd, 0, 0, &tv);              // if socket is ready or timer expired 
		GetSystemTime(&st2);
		int passed = st2.wMilliseconds - st1.wMilliseconds;
		if (passed < 0) passed += 1000;
		if (!s) //timer expired
        {
			if (decrement_timer(tl) != -1){
				while (tl != NULL && tl->timer == 0)
				{
					if (tl->seq_nr == window_start || TRUE)   //passed Timer is lower window border -> push window
					{
						ans.AnswType = AnswNACK;
						ans.SeNo = tl->seq_nr;
						sendAnswer(&ans);						//send NACK
						tl = del_timer(tl, tl->seq_nr, FALSE);
						tl = add_timer(tl, TIMEOUT_MULTI, ans.SeNo);
					}
				}
				if (window_start + window_size > timerNumber)
				{
					timerNumber++;
					tl = add_timer(tl, TIMEOUT_MULTI, timerNumber);
				}
				continue;
			}
			if (window_start + window_size > timerNumber)
			{
				timerNumber++;
				tl = add_timer(tl, TIMEOUT_MULTI, timerNumber);
			}
			continue;
        }
        if (s == SOCKET_ERROR)
        {
            fprintf(stderr, "select() failed: error %d\n", WSAGetLastError());
            exit(7);
        }
        getRequest(&req);
		if (passed<300 && passed >290) decrement_timer(tl);
		if (req.ReqType == ReqClose)
		{
			if (!req.SeNr)
			{
				printf("Received Close with SeNr 0\nexiting\n");
				getchar();
				exit(1);
			}
			if (rc)                                     // if we're still missing packets
			{
				printReq(req, 5);
				continue;                               // continue sending a NACK
			}
			answreturn(&ans, &req, expectedSequence);    // else send a CloseACK
			if (ans.AnswType != AnswClose)
			{
		//		ans.SeNo = 5;
		//		sendAnswer(&ans);
				continue;
			}
			sendAnswer(&ans);
			stay = 0;                                   // get out of loop
		}
		if (req.SeNr > expectedSequence)
        {
            if (req.ReqType == ReqData)
            {
                insert(&rc, &req);       // put packet in cache
                printReq(req, 2);
            }
			tl = del_timer(tl, req.SeNr, TRUE);
		    answreturn(&ans, &req, expectedSequence);
            sendAnswer(&ans);
			tl = del_timer(tl, expectedSequence, TRUE);
			tl = add_timer(tl, TIMEOUT_MULTI, expectedSequence);
		    continue;
        }
        if (req.SeNr < expectedSequence) // NACK from other server, we already have this packet
        {
            printReq(req, 6);
            continue;
        }
        if (req.ReqType == ReqData)
        {
            strl = addtolist(strl, req.name);            //store string of packet in strlist
            expectedSequence++;
			window_start++;
			tl = del_timer(tl, req.SeNr, TRUE);
			while (rc && peek(rc) == expectedSequence)    //if any element is in cache and the first one is our expected 
            {
                cache* ca = get(&rc);                   // get it out
                strl = addtolist(strl, ca->req.name);   // and store its string
                printReq(ca->req, 3);
                free(ca);
                ca = NULL;
				window_start++;
                expectedSequence++;
            }
			if (rc)
			{
				tl = del_timer(tl, window_start + 1, TRUE);
				tl = add_timer(tl, 3, window_start + 1);
				answreturn(&ans, &req, window_start + 1);
				sendAnswer(&ans);
			}
			continue;
        }

    }
    int w;
    if (w=writefile(filename, strl)) fprintf(stderr, "writefile() returned %i", w);
    freelist(strl);
	getchar();
    return 0;
}
