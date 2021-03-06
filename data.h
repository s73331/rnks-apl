/* Data Declarations */
/* for Client and Server */


#define INTERVAL 300             // in Milliseconds
#define INT_MS INTERVAL*1000         // in Microseconds
#define TIMEOUT_MULTI 3                   // must be a multiple of TIMEOUT_INT
#define MAX_WINDOW 10               // maximum windows size
#define MAX_SEQNR 2*MAX_WINDOW-1    /* maximum sequence number --> real maximum sequence number
                                       is 2 times of the choosen real window size!!*/
#define MAX_BUFFER 2*MAX_WINDOW     // packets must be stored for retransmission
#define PufferSize 256

extern char *errorTable[];

struct request {
    unsigned char ReqType;
    #define ReqHello 'H'    // ReqHello
    #define ReqData  'D'    // ReqData '0'
    #define ReqClose 'C'    // ReqClose
    long FlNr;              /* Data length (line of text) in Byte ; */
    unsigned long SeNr;     /* Sequence Number (== 0) beginn of file */
    char name[PufferSize];  // Data --> line of text
};

struct answer {
    unsigned char AnswType;
    #define AnswHello 'H'
    #define AnswOk    '0'
    #define AnswNACK  'N'   //Multicast Group receiver sends
                            //negative acknowledgement if received packet is not in order
    #define AnswClose 'C'
    #define AnswErr   0xFF
    unsigned FlNr;          /* if it is a Hello Ack packet we might send our window size;
                               sender/client chooses minimum window size of all servers*/
    unsigned SeNo;
    #define ErrNo SeNo      /* are identical */
};

typedef struct _strlist
{
    char str[PufferSize];
    struct _strlist* next;
}strlist;

typedef struct _cache
{
    struct request req;
    struct _cache* next;
}cache;