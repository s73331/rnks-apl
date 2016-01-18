#include <WinSock2.h>
#include <stdio.h>
void fd_reset(fd_set* fd, SOCKET ConnSocket)
{
    if (!fd) return;
    FD_ZERO(fd);                    // remove everything from fd 
    FD_SET(ConnSocket, fd);         // add ConnSocket
    if (!FD_ISSET(ConnSocket, fd))  // check for success
    {
        fprintf(stderr, "FD_SET failed\nexiting...");
        exit(1);
    }
}