/*
    sent shall be non-zero if sent and zero if received
*/
void printAns(struct answer answ, int sent);

/*
    flag:
            0 - received
            1 - sent
            2 - added to cache
            3 - resolved from cache
            4 - manipulating
            5 - ignored
*/
void printReq(struct request req, int flag);