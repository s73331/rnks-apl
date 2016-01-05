/* win32 Konsole Version - connectionless */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "data.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ctype.h>

/*struct request req;*/
struct request req;

/*** Declaration socket descriptor "s" ***/
SOCKET ConnSocket;

/*** Deklaration of socket address "local" static ***/
//static struct sockkaddr_in6 localAddr;
struct sockaddr *sockaddr_ip6_local=NULL; 

/*** Deklaration of socket address "remote" static ***/
static struct sockaddr_in6 remoteAddr;;

int initServer(char *MCAddress, char *Port) {
    int trueValue = 1, loopback = 0; //setsockopt
    int val,i=0;
    int addr_len;
    struct ipv6_mreq mreq; //mu1ticast address
    struct addrinfo *resultLocalAddress = NULL,*resultMulticastAddress = NULL,*ptr = NULL, hints;
    WSADATA wsaData;
    WORD wVersionRequested;
    wVersionRequested = MAKEWORD(2,1);
    if( WSAStartup( wVersionRequested,&wsaData ) == SOCKET_ERROR ){
        printf("SERVER: WSAStartup() failed!\n");
        printf("        error code: %d\n",WSAGetLastError());
        exit(-1);
    }
    /*** Create Socket, ***/
    /*** connectionless service, address family INET6 ***/
    ConnSocket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if (ConnSocket < 0 ) {
        fprintf(stderr,"C1ient: Error Opening socket: Error %d\n",
            WSAGetLastError());
        WSACleanup();
        exit(-1);
    }
    /* Initialize socket */
    /* Reusing port for several server listening on same multicast addr and port
    (if we are testing on local machine only */
    setsockopt(ConnSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&trueValue, sizeof(trueValue));
    /* Resolve multicast group address to join mc group*/
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_INET6;
    hints.ai_flags  = AI_NUMERICHOST;
    if ( getaddrinfo(MCAddress, NULL, &hints, &resultMulticastAddress) != 0 ){
        fprintf(stderr,"getaddrinfo MCAddress failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        exit(-1);
    }
    /* Resolve local address (anyaddr) to bind*/
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family   = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags    = AI_PASSIVE; //loca1host
    val = getaddrinfo(NULL, Port, &hints, &resultLocalAddress);
    if ( val != 0 ) {
        printf("getaddrinfo localAddress failed with error: %d\n", val);
        WSACleanup();
        exit(-1);
    }
    // Retrieve the address and print out the hex bytes
    for(ptr=resultLocalAddress; ptr != NULL ;ptr=ptr->ai_next) {
        printf("getaddrinfo response %d\n", i++);
        printf("\tFlags: 0x%x\n", ptr->ai_flags);
        printf("\tFamily: ");
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
                addr_len= ptr->ai_addrlen;
                break;
            default:
                printf("other %ld\n", ptr->ai_family);
                break;
        }
    
    }
    /*** Bind Socket ***/    
    printf("in bind\n");
    
    if (bind(ConnSocket, sockaddr_ip6_local, addr_len) == SOCKET_ERROR) {
        fprintf(stderr,"bind() failed: error %d\n", WSAGetLastError());
        WSACleanup();
        exit (-1);
    }
    /* Specify the multicast group */
    memcpy(&mreq.ipv6mr_multiaddr, &((struct sockaddr_in6*)(resultMulticastAddress->ai_addr))->sin6_addr,
    sizeof(mreq.ipv6mr_multiaddr));
        /* Accept multicast from any interface */
    // scope ID from Int. -9 to get scopeid :netsh int ipv6 sh addr or ipconfig —all
    mreq.ipv6mr_interface = 3; // my w8 Laptop
    /* Join the multicast address (netsh interface ipv6 show joins x)*/
    if ( setsockopt(ConnSocket, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char*) &mreq, sizeof(mreq)) != 0 ) {
        fprintf(stderr,"setsockopt(IPV6_JOIN_GROUP) failed %d\n",WSAGetLastError());
    WSACleanup();
    exit (-1);
    }
    freeaddrinfo(resultLocalAddress);
    freeaddrinfo(resultMulticastAddress);
    return(0);
}
struct request *getRequest() {
    static long seq_number = 0; // expected seq_number in byte
    int recvcc;                 /* Length of received Message */
    int remoteAddrSize = sizeof(struct sockaddr_in6);
    /* Receive a message from a socket */
    recvcc = recvfrom(ConnSocket, (char *)&req, sizeof(req), 0, (struct sockaddr *) &remoteAddr,
        &remoteAddrSize);
    if (recvcc == SOCKET_ERROR) {
        fprintf(stderr,"recv() failed: error %d\n",WSAGetLastError());
        closesocket(ConnSocket);
        WSACleanup();
        exit(-1);
    }
    if (recvcc == 0)
    {
        printf("Client closed connection\n");
        closesocket(ConnSocket);
        WSACleanup();
        exit(-1);
    }
    return(&req);
}
void sendAnswer(struct answer *answ) {
    /*** Send NACK back to Unicast Address ***/
    int w;
    w = sendto(ConnSocket, (const char *)answ, sizeof(*answ), 0, (struct sockaddr *)&remoteAddr,
        sizeof(remoteAddr));
    if (w == SOCKET_ERROR){
        fprintf(stderr,"send() failed: error %d\n",WSAGetLastError());
    }
}
int exitServer() {
/*** Close Socket ***/
    closesocket(ConnSocket);
    printf("in exit srver\n");
    if (WSACleanup()==SOCKET_ERROR){
        printf("SERVER: WSACleanup() failed!\n");
        printf("        error code: %d\n“,WSAGetLastError()");
        exit(-1);
    }
    return(0);
}

 
