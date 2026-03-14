#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>


int
main(void)
{
    struct addrinfo hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    /*
     * If we want to establish a connection with a particular IP address or domain
     * as a client, we can do so by giving the first argument of getaddrinfo and
     * not setting the hints.ai_flags to AI_PASSIVE
     */
    int status;
    if ((status = getaddrinfo("www.example.com", "3490", &hints, *res)) == -1) {
        fprintf(stderr, "Error: %s\n", status);
        exit(EXIT_FAILURE);
    }

    // do the work

    freeaddrinfo(res);
}
