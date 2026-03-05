#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>


/*
  int getaddrinfo(const char *node,
                    const char *service,
                    const struct addrinfo *hints,
                    struct addrinfo **res)

    node -> host name to connect to or an IP address
    service -> can be a port no. or a particular name like "http" or "ftp"
    hints -> acts as a template for the resuling linked list
 */


int
main(void)
{
    struct addrinfo hints;
    struct addrinfo *res;  // stores the resulting linked list

    // fill in the fields of hints
    memset(&hints, 0, sizeof(hints));  // make sure every bytes is 0
    hints.ai_family   = AF_UNSPEC;    // can use any of IPv4 and IPv6
    hints.ai_socktype = SOCK_STREAM;  // use tcp stream socket
    hints.ai_flags    = AI_PASSIVE;   // fill in my ip for me

    int status;
    if ((status = getaddrinfo(NULL, "3409", &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    // ...
    // do the work
    // ...

    // free the linked list pointed by res
    freeaddrinfo(res);
}
