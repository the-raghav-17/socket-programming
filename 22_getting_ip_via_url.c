#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>


void *
get_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


int
main(void)
{
    struct addrinfo *res;
    int rv;

    if ((rv = getaddrinfo("www.google.com", "80", NULL, &res)) == -1) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(EXIT_FAILURE);
    }

    struct addrinfo *p;
    for (p = res; p != NULL; p = p->ai_next) {
        char addr_str[INET6_ADDRSTRLEN];
        inet_ntop(p->ai_family, get_addr(p->ai_addr),
                  addr_str, sizeof(addr_str));

        printf("IP: %s\n", addr_str);
    }

    exit(EXIT_SUCCESS);
}
