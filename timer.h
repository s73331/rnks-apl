typedef struct timeouts
{
	unsigned long seq_nr;
	unsigned long timer;    // number of TIMEOUT_INT for retransmitting
	struct timeouts* next;
} timeouts2;
struct timeouts* addtimer(struct timeouts *list, int timer_val, unsigned long seq_nr); //add in order
