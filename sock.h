/*
Prepares fd with and only with ConnSocket.

The input value of fd must be a pointer to an existing fd_set.
*/
void fd_reset(fd_set* fd, SOCKET ConnSocket);