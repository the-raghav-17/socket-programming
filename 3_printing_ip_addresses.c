#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


int
main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Invalid invocation. Use: ./a.out <domain-name>\n");
        exit(EXIT_FAILURE);
    }

    struct addrinfo  hints;  // template
    struct addrinfo *res;    // resulting linked list

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status;
    if ((status = getaddrinfo(argv[1], NULL, &hints, &res)) == -1) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    }

    char ipstr[INET6_ADDRSTRLEN];  // stores the string for the ip address.
                                   // is large enough to hold either of ipv4 or ipv6

    printf("IP address for %s:\n\n", argv[1]);

    for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        // get the pointer to address itself
        // different fields in IPv4 and IPv6
        if (p->ai_family == AF_INET) {  // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
            addr  = &(ipv4->sin_addr);
            ipver = "IPv4";
        }
        else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) p->ai_addr;
            addr  = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
        printf("  %s: %s\n", ipver, ipstr);
    }

    freeaddrinfo(res);
}
