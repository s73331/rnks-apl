typedef struct timeouts
{
	unsigned long seq_nr;
	unsigned long timer;    // number of TIMEOUT_INT for retransmitting
    struct timeouts* next;
} timeouts2;

/*
    If list was NULL, creates a new item with the parameters and assigns it to list.
    Otherwise places a timer at the correct position in the list, making its timer to match timer_val to the sum of it and its predecessors.
    
    Always returns list.
*/
struct timeouts* add_timer(struct timeouts *list, unsigned int timer_val, unsigned long seq_nr);

/*
    Removes the timer with the given seq_nr.
    If there is no timer with the given seq_nr, nothing happens.

    return: 
            0       if list is NULL
            list    otherwise
        NOTE: list can be NULL
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