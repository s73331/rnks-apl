/* Win32 Konsole Version -  connectionless */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <ctype.h>

#include "data.h"
#include "config.h"
#include "toUdp.h"
#include "print.h"
#include "file.h"
#include "local.h"
#pragma comment(lib, "Ws2_32.lib")				// necessary for the WinSock2 lib

#define BUFLEN 512	
struct request req;

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
	mreq.ipv6mr_interface = INADDR_ANY; //my w8 Laptop

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

struct request *getRequest() {
	//printf("in get req\n");
	//static long seq_number = 0;  //expected seq_number in byte
	int recvcc;		     /*Length of received Message */
	int remoteAddrSize = sizeof(struct sockaddr_in6);

	/* Receive a message from a socket */
	recvcc = recvfrom(ConnSocket, (char*)&req, sizeof(req), 0, (struct sockaddr *) resultMulticastAddress, &remoteAddrSize);
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
    printReq(req, 0);
	return(&req);
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

struct answer *answreturn(struct request *reqPtr, int *sqnr_counter, int *window_size, int *drop_pack_sqnr)
{
	struct answer* answ =malloc(sizeof(answ));

	switch (reqPtr->ReqType)
	{
	case ReqHello:
		answ->AnswType = AnswHello;
		answ->FlNr = reqPtr->FlNr;
		answ->SeNo = reqPtr->SeNr;
		(*window_size) = reqPtr->FlNr;
		return(answ);
		break;
    case ReqClose:
        answ->AnswType = AnswClose;
        answ->FlNr = reqPtr->FlNr;
        answ->SeNo = reqPtr->SeNr;
        (*window_size) = reqPtr->FlNr;
        return(answ);
        break;
	default:
		printf("default");
        return NULL;
		break;
	}
}

int main() {
    char* filename = FILE_TO_WRITE;
	initServer(DEFAULT_SERVER, DEFAULT_PORT);
	struct answer *ans;
    strlist* strl = NULL;
    strlist* last = NULL;
	int sqnr_counter = 1, window_size = 1, drop_pack_sqnr, drop = 0;
    //int c, v;
    int stay = 1;
    while (stay)
    {
        struct request *req = getRequest();
        if (req->ReqType == ReqHello)
        {
            ans = answreturn(req, &sqnr_counter, &window_size, &drop_pack_sqnr);
            sendAnswer(ans);
            continue;
        }
        if (req->ReqType == ReqData)
        {
            addtolist(strl, last, req->name);
            continue;
        }
        if (req->ReqType == ReqClose)
        {
            ans = answreturn(req, &sqnr_counter, &window_size, &drop_pack_sqnr);
            sendAnswer(ans);
            stay = 0;
        }
    }
    int w;
    if (w=writefile(filename, strl)) fprintf(stderr, "writefile() returned %i", w);
	fflush(stdin);
	getchar();
	return 0;
}
/*
int main3(int argc, char *argv[])
{
	FILE *fp;
	int i;
	int status;
	char buffer[256];
	int packetnummer = 0;
	char *Server = DEFAULT_SERVER;
	char *Filename = "";
	char *Port = DEFAULT_PORT;
	char *Window_size = 10;

	char buf[BUFLEN];
	int recv_len;



	status = 0;

	//Parameter ueberpruefen
	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			if (((argv[i][0] == '0') || (argv[i][0] == '/')) && (argv[i][1] != 0) && (argv[i][2] == 0)) {
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
				case 'w':			//Window Size
					if (argv[i + 1])
					{
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

	//Server initialisieren
	i = initServer(Server, Port);
	printf("Addr:\t%s\nPort:\t%s\n", Server, Port);
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