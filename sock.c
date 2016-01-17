#include <WinSock2.h>
#include <stdio.h>
void fd_reset(fd_set* fd, SOCKET ConnSocket)
{
    FD_ZERO(fd);
    FD_SET(ConnSocket, fd);
    if (!FD_ISSET(ConnSocket, fd))
    {
        fprintf(stderr, "FD_SET failed\nexiting...");
        exit(1);
    }
}