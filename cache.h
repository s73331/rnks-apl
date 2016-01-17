/*
    Inserts a Request into a Requestcache.
*/
void insert(struct _cache** start, struct request* newelem);
/*
    Returns the lowest sequence number in the cache.
*/
int peek(struct _cache* start);
/*
    Get the element with the lowest sequence number.
*/
struct _cache* get(struct _cache** start);