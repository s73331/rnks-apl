typedef struct timeouts
{
	unsigned long seq_nr;
	unsigned long timer;    // number of TIMEOUT_INT for retransmitting
	struct timeouts* next;
} timeouts2;

/*
    Always returns list.
    If list was NULL, creates a new item with the parameters and assigns it to list.
*/
struct timeouts* addtimer(struct timeouts *list, int timer_val, unsigned long seq_nr); //add in order

/*
    return: 
            0       if list is NULL
            list    otherwise
*/
struct timeouts* del_timer(struct timeouts *list, unsigned long seq_nr);

/*
    Decrements list->timer by one.
    return: 
            -1 list is NULL
             0 list->timer after decrement is 0
             1 list->timer after decrement is greater than 0
*/
int decrement_timer(struct timeouts *list);