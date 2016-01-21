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
	setsockopt(ConnSocket, LEVEL, OPTNAME, (char *)&trueValue, sizeof(trueValue));

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
	mreq.ipv6mr_interface = IPV6MR_INTERFACE; //my w8 Laptop

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
        printReq(*req, 4);
    else printReq(*req, 0);
	return;
}

void sendAnswer(struct answer * answ) {
	/****************************************************/
	/*** Send NACK back to Unicast Address            ***/
	/****************************************************/
	int w;
    w = sendto(ConnSocket, (const char *)answ, sizeof(*answ), 0, (struct sockaddr *)resultMulticastAddress, sizeof(struct sockaddr_in6));
	//	recvcc = recvfrom(ConnSocket, (char*)&req, sizeof(req), 0, (struct sockaddr *) resultMulticastAddress, &remoteAddrSize);
    //w = sendto(ConnSocket, (const char*)answ, sizeof(*answ), 0, resultLocalAddress, resultMulticastAddress->ai_addrlen);
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
    long int window_size = DEFAULT_WINDOW;
    //Parameter ueberpruefen
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
    while (stay)
    {   
        fd_reset(&fd, ConnSocket);                  // add ConnSocket to struct, needs to be redone before every select
        if (!tl) tl = add_timer(tl, TIMEOUT, req.SeNr);
        tv.tv_usec = (long)((tl->timer)*TO+150*1000);// add 150millis as otherwise our NACK would cross their datapacket
        s = select(0, &fd, 0, 0, &tv);              // if socket is ready or timer expired 
        if (!s) //timer expired
        {
            tl = del_timer(tl, req.SeNr, FALSE);
            answreturn(&ans, &req, expectedSequence);
            sendAnswer(&ans);
            continue;
        }
        if (s == SOCKET_ERROR)
        {
            fprintf(stderr, "select() failed: error %d\n", WSAGetLastError());
            exit(7);
        }
        if (errorQuota >= 0 && (int)(errorQuota*RAND_MAX) > rand() || errorQuota<0 && IGNORE_ARRAY_SIZE > req.SeNr && IGNORE_DATA[req.SeNr])
        {
            manipulating = 1;
            getRequest(&req);
            manipulating = 0;
            continue;
        }
        tl = del_timer(tl, req.SeNr, TRUE);
        getRequest(&req);
        if (req.SeNr > expectedSequence)
        {
            if (req.ReqType == ReqData)
            {
                insert(&rc, &req);       // put packet in cache
                printReq(req, 2);
            }
            answreturn(&ans, &req, expectedSequence);
            sendAnswer(&ans);
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
            while (rc && peek(rc) == expectedSequence)    //if any element is in cache and the first one is our expected 
            {
                cache* ca = get(&rc);                   // get it out
                strl = addtolist(strl, ca->req.name);   // and store its string
                printReq(ca->req, 3);
                free(ca);
                ca = NULL;
                expectedSequence++;
            }
            continue;
        }
        if (req.ReqType == ReqClose)
        {
            if (rc)                                     // if we're still missing packets
            {
                printReq(req, 5);
                continue;                               // continue sending a NACK
            }
            answreturn(&ans, &req, expectedSequence);    // else send a CloseACK
            sendAnswer(&ans);
            stay = 0;                                   // get out of loop
        }
    }
    int w;
    if (w=writefile(filename, strl)) fprintf(stderr, "writefile() returned %i", w);
    freelist(strl);
    return 0;
}

	//File erstellen
	//fp = fopen(Filename, "w+");

	/*while (1)
	{
	packetnummer = 0;

	//auf hello warten
	if (getRequest() == ReqHello)
	{
	sendAnswer(AnswHello);
	status = 1;

	//Daten empfangen
	while (status != 2)
	{
	if (getRequest()->ReqType == ReqData && getRequest()->SeNr == packetnummer)
	{
	//richtiges Packet mit richtiger SeNr
	sendAnswer(AnswOk);
	fputs(getRequest()->name, fp);
	packetnummer++;
	}

	if (getRequest()->SeNr != packetnummer)
	{
	//Packet nicht in reihenfolge angekommen
	sendAnswer(packetnummer);
	}

	if (getRequest()->ReqType == ReqClose)
	{
	//Sender sendet "close"
	status = 2;
	sendAnswer(AnswClose);
	}

	}
	}
	}

	int remoteAddrSize = sizeof(struct sockaddr_in6);
	char message[BUFLEN];

	while (1)
	{




		printf("Waiting for data...\n");
		fflush(stdout);

		//clear the buffer by filling null, it might have previously received data
		memset(buf, '\0', BUFLEN);

		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(ConnSocket, buf, BUFLEN, 0, resultMulticastAddress->ai_addr, resultMulticastAddress)) == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}



		printf("Data: %s\n", buf);



	}
	exitServer();

	fflush(stdin);
	getchar();


	return 0;
}

*/
