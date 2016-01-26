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

SOCKET ConnSocket;                                                           //Declaration socket descriptor "s" 
static struct sockaddr_in6 localAddr;                                        //Declaration of socket addresss "local" static
struct sockaddr *sockaddr_ip6_local = NULL;
static struct addrinfo *resultMulticastAddress = NULL;                       //Declaration of socket address "remote" static


void Usage(char *ProgName)          
{                                                   /*** Display how to use the program ***/
    fprintf(stderr, P_Message_1);
    fprintf(stderr, P_Message_6, ProgName);
    fprintf(stderr, P_Message_7, (DEFAULT_SERVER == NULL) ? "loopback address" : DEFAULT_SERVER);
    fprintf(stderr, P_Message_8, DEFAULT_PORT);
    fprintf(stderr, P_Message_9);
    fprintf(stderr, P_Message_10);
    fprintf(stderr, P_Message_11);
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

int initClient(char *MCAddress, char *Port) {
    int trueValue = 1, loopback = 1; // for setsockopt
    int val, i = 0;
    int addr_len;
    struct ipv6_mreq mreq; //define multicast address pickup struct
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
    /*** Create Socket,                                ***/
    /*** connectionless service, addresss family INET6 ***/
    /****************************************************/
    ConnSocket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if (ConnSocket < 0) {
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
        fprintf(stderr, "getaddrinfo MCAddress failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        exit(-1);
    }


    /* Resolve local address to bind */
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    hints.ai_flags = AI_PASSIVE; 		// localhost

    val = getaddrinfo(NULL, Port, &hints, &resultLocalAddress);

    if (val != 0) {
        fprintf(stderr, "getaddrinfo localAddress failed with error: %d\n", val);
        WSACleanup();
        exit(-1);
    }

	
    /* Retrive the address and print  out the hex bytes */
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


    /****************************************************/
    /* Specify the multicast group                      */
    /****************************************************/
    memcpy(&mreq.ipv6mr_multiaddr, &((struct sockaddr_in6*)(resultMulticastAddress->ai_addr))->sin6_addr, sizeof(mreq.ipv6mr_multiaddr));

	
    /* Accept multicast from any interface 
       scope ID from Int. -> to get scopeid :netsh int ipv6 sh addr or ipconfig -all */
    mreq.ipv6mr_interface = IPV6MR_INTERFACE;

	
    /*Join the multicast address frame (netsh interface ipv6 show joins x) */
    if (setsockopt(ConnSocket, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char*)&mreq, sizeof(mreq)) < 0)
    {
        fprintf(stderr, "setsockopt(IPV6_JOIN_GROUP) failed %d\n", WSAGetLastError());
        WSACleanup();
        fflush(stdin);
        getchar();
        exit(-1);
    }

	
    freeaddrinfo(resultLocalAddress);
    //freeaddrinfo(resultMulticastAddress);

    return(0);
}

int makeRequest(struct request* req, strlist* strli, unsigned long* lastSeNr, int* lastData)
{
    if (*lastData)                      // if we have read the last line
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
    gl = getline(strli, req->SeNr - 1, buf); 	//-1, as SeNo=0 is not data
    if (gl < 0)
    {
        fprintf(stderr, "getting line failed with error code %i\nrequested line: %i\nexiting...", gl, req->SeNr - 1);
        exit(6);
    }
    strncpy(req->name, buf, PufferSize);
    return gl;
}

int sendRequest(struct request* req, strlist* strli, unsigned long* lastSeNr, int* lastData, SOCKET ConnSocket, double errorQuota)
{
    int ret = makeRequest(req, strli, lastSeNr, lastData);
    
	/* Error Quota implementation with fake packets */
	if ((errorQuota > 0 && errorQuota <= 1 && (int)(errorQuota*RAND_MAX) > rand() || errorQuota == 2 && NOSEND_ARRAY_SIZE > req->SeNr && NOSEND_DATA[req->SeNr] )&& req->ReqType != ReqClose)
        printReq(*req, 4);  	// act like we sent the packet
    else
    {                       
		// Actually send the packet
        int w = sendto(ConnSocket, (const char*)req, sizeof(*req), 0, resultMulticastAddress->ai_addr, resultMulticastAddress->ai_addrlen);
        if (w == SOCKET_ERROR) {
            fprintf(stderr, "send() failed: error %d\n", WSAGetLastError());
            exit(1);
        }
        printReq(*req, 1);
    }
    return ret;
}

sendHello(struct request* req, SOCKET ConnSocket, int window_size)
{
    req->FlNr = window_size;
    req->ReqType = ReqHello;
    int w = sendto(ConnSocket, (const char*)req, sizeof(*req), 0, resultMulticastAddress->ai_addr, resultMulticastAddress->ai_addrlen);
    if (w == SOCKET_ERROR) {
        fprintf(stderr, "send() failed: error %d\n", WSAGetLastError());
        exit(1);
    }
    printReq(*req, 1);
}

sendDataAgain(struct request* req, struct answer ans, strlist* strli, SOCKET ConnSocket)
{
    req->SeNr = ans.SeNo;
    req->ReqType = ReqData;
    char buf[PufferSize + 1];
    buf[PufferSize] = 0;
    int gl = getline(strli, req->SeNr - 1, buf);
    if (gl == -2)            							//Close NACK requested again
        req->ReqType = ReqClose;
    else if(gl == -1)
    {
        fprintf(stderr, "getting line failed with error code %i\nrequested line: %i\nexiting...", gl, req->SeNr - 1);
        exit(6);
    }else
        strncpy(req->name, buf, PufferSize);

    int w = sendto(ConnSocket, (const char*)req, sizeof(*req), 0, resultMulticastAddress->ai_addr, resultMulticastAddress->ai_addrlen);
    if (w == SOCKET_ERROR) {
        fprintf(stderr, "send() failed: error %d\n", WSAGetLastError());
        exit(1);
    }
    printReq(*req, 1);
}

int recvfromw(SOCKET ConnSocket, char* buf, size_t len, int flags, struct sockaddr* from, int* fromlen)
{
    int ret = recvfrom(ConnSocket, buf, len, flags, from, fromlen);
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
    unsigned int window_size = 1;
    double errorQuota = 0;
    char* port = DEFAULT_PORT;
    char* server = DEFAULT_SERVER;
    char* filename = FILE_TO_READ;

    /****************************************************/
    /*** User Interface ***/
    /****************************************************/

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
                case 's':                       //Server Address
                    if (argv[i + 1]) {
                        if (argv[i + 1][0] != '-') {
                            server = argv[++i];
                            break;
                        }
                    }
                    Usage(argv[0]);
                    break;
                case 'p':                       //Server Port
                    if (argv[i + 1]) {
                        if (argv[i + 1][0] != '-') {
                            port = argv[++i];
                            break;
                        }
                    }
                    Usage(argv[0]);
                    break;
                case 'f':                       //File Name
                    if (argv[i + 1]) {
                        if (argv[i + 1][0] != '-') {
                            filename = argv[++i];
                            if (strlen(filename) > 259 || strlen(filename) < 1) Usage(argv[0]);
                            break;
                        }
                    }
                    Usage(argv[0]);
                    break;
                case 'w':                       //Window Size
                    if (argv[i + 1])
                    {
                        if (argv[i + 1][0] != '-') {
                            window_size = strtol(argv[++i], 0, 0);
                            if (window_size < 1 || window_size>10) Usage(argv[0]);
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

	
    /****************************************************/
    /*** END User Interface ***/
    /****************************************************/

    struct timeouts* tl = NULL;                 // struct to store our timeouts (relative geordnete verkettete Liste)
    int markedWindow[10] = { 0 };               // Array to mark timeouts which are not at the bottom end of the window
    unsigned int windowBase;                    // Base (lower border) of the Window | window_size defines window
    struct answer ans;                          // struct for NACK-Answers
    ans.SeNo = 0;
    SYSTEMTIME st, st2;
    struct request req;                         // struct for Send
    req.SeNr = 0;
    int closeAckRecvd = 0;                      // Counter of recieved CloseAcks
    fd_set fd;                                  // Socket handover to select
    struct timeval tv;                          // struct for select-timevals
    tv.tv_sec = 0;
    tv.tv_usec = INT_MS;						// set Time of one interval (default: 3000 ms)
    strlist* strli = NULL;                      // struct for our file, which turned into a list of char-arrays (note: NOT strings, not even C-Strings)
    unsigned long lastSeNr = 0;                 // last sequence number we ever sent
    int lastData = 0;                           // 0 still have data, 1 have just read last line, 2 have read last line before
    int helloAckRecvd = 0;                      // Counter of recieved HelloAcks = connected Servers / Listeners

	
    /****************************************************/
    /*** Establish a Connection ***/
    /****************************************************/

    initClient(server, port);
    if (readfilew(filename, &strli))
        fprintf(stderr, "closing file failed\ncontinuing...\n");
	
    while (!helloAckRecvd)                                     			// Send Hello's until we have recieved Listeners            
    {
        sendHello(&req, ConnSocket, window_size);
        fd_reset(&fd, ConnSocket);
		
        int s = select(0, &fd, 0, 0, &tv);       						// select if socket is read or a interval has passed
		
        if (!s)                                                                             
        {
            continue;													// timer expired no Hello recieved
        }	
        else															// something on the select
        {
            if (s == SOCKET_ERROR)
            {
                fprintf(stderr, "select() failed: error %d\n", WSAGetLastError());
                exit(7);
            }
            recvfromw(ConnSocket, (char*)&ans, sizeof(ans), 0, 0, 0);   // Get the Answer    
            if (ans.AnswType == AnswHello)
            {
                helloAckRecvd++;
            }
            else{
                fprintf(stderr, "erwartet 'AmswHello' als ans.AnswType\n");
            }
        }
    }
    windowBase = 1;                                                     // Window initialized with base = 1

	
    /****************************************************/
    /*** Sending und Recieving - Loop 				  ***/
    /****************************************************/

    int stay = 1;
	
    while (stay)															 // send and recieve until #closeAck == #HelloAck
    {
        GetSystemTime(&st);
        //printf("%02d.%03d \t %02d\n", st.wSecond, st.wMilliseconds, windowBase);
        if (req.ReqType != ReqClose)                                         // the only time when we're truly waiting (NACK or CloseACK)
        {
            fd_reset(&fd, ConnSocket);                                       // reset our fd for select
            tv.tv_usec = INT_MS;					                         // set Time of one interval (default: 3000 ms)
            GetSystemTime(&st);
            int s = select(0, &fd, 0, 0, &tv);                               // select if socket is read or a interval has passed
            if (!s)                                                          // interval passed without the socket recieved anything                
            {
                /****************************************************/
                /*** Timer-Management after an interval ***/
                /****************************************************/

                if (decrement_timer(tl) != -1){                              // decrement when there are timers
                    while (tl != NULL && tl->timer == 0)                     // remove passed timers, mark or push-forward the window
                    {
                        if (tl->seq_nr == windowBase)                        // timer for base has passed -> push-forward the window
                        {
                            windowBase++;
                            tl = del_timer(tl, tl->seq_nr, FALSE);
                            for (int i = 0; i < 10; i++)                     // wenn a new base is already marked -> push-forward the window
                            {
                                if (markedWindow[i] == windowBase)
                                {
                                    windowBase++;
                                    markedWindow[i] = 0;
                                }
                            }
                        }
                        else{                                                // Timeout SN != BaseSN -> Add mark that this value has a timeout
                            int i = 0;                                       
                            while (markedWindow[i] != 0)
                            {
                                i++;
                            }
                            markedWindow[i] = tl->seq_nr;
                            tl = del_timer(tl, tl->seq_nr, FALSE);

                        }
                    }
                }

                /****************************************************/
                /*** Usual Datasend								  ***/
                /****************************************************/

                if (lastSeNr < windowBase + (window_size - 1))               // Next packet is in the window -> send it and add timer
                {
                    lastData += sendRequest(&req, strli, &lastSeNr, &lastData, ConnSocket, errorQuota);
                    tl = add_timer(tl, TIMEOUT_MULTI, req.SeNr);
                }
                else											             // lost interval (waiting for new base or NACK)
                {
                    printReq(req, 7);
                }
                continue;
            }
            if (s == SOCKET_ERROR)
            {
                fprintf(stderr, "select() failed: error %d\n", WSAGetLastError());
                exit(7);
            }
        }

        /****************************************************/
        /*** Data-Recieve -> Interval interrupted ***/
        /****************************************************/

        recvfromw(ConnSocket, (char*)&ans, sizeof(ans), 0, 0, 0);
        switch (ans.AnswType)
        {
        case AnswHello:
            helloAckRecvd++;
            break;
        case AnswNACK:
            if (ans.SeNo >= windowBase && ans.SeNo <= windowBase + (window_size - 1))
            {                                                                // NAck demands packet from current window
                tl = del_timer(tl, ans.SeNo, TRUE);
                GetSystemTime(&st2);                                         // fill time until the true interval-end
                int passed = st2.wMilliseconds - st.wMilliseconds;
                if (passed < 0) passed += 1000;
                if (passed < 300) Sleep(300 - passed);
                sendDataAgain(&req, ans, strli, ConnSocket);
                tl = add_timer(tl, TIMEOUT_MULTI, req.SeNr);
            }
            else
            {												                 // NAck demands packet from other window
                lastData = 2;								                 // set Flag for connection close -> send closeAck
                req.ReqType = 'O';
                lastSeNr = 0;
                printReq(req, 8);
                lastData += sendRequest(&req, strli, &lastSeNr, &lastData, ConnSocket, errorQuota);
                exit(5);
            }
            continue;
        case AnswClose:
            closeAckRecvd++;
            if (helloAckRecvd == closeAckRecvd)
                stay = 0;
            continue;
        default:
            fprintf(stderr, "not recognized ans.AnswType\nexiting\n");
            exit(1);
        }
    }
    freelist(strli);
    closesocket(ConnSocket);
    WSACleanup();
    return 0;
}
