#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>


#define PORT "3490"
#define MAX_DATASIZE 100


void *
get_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


int
main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: ./client hostname\n");
        exit(EXIT_FAILURE);
    }

    struct addrinfo  hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status;
    if ((status = getaddrinfo(argv[1], PORT, &hints, &res)) == -1) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    // Now traverse the list and connect to the first node we can
    struct addrinfo *p;
    int sockfd;

    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }
        break;
    }

    /* freeaddrinfo(res); */

    if (p == NULL) {
        fprintf(stderr, "client: connection failed\n");
        exit(EXIT_FAILURE);
    }

    char s[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, get_addr((struct sockaddr *)p->ai_addr), s, sizeof(s));
    printf("client: connected to %s\n", s);

    // Now we'll recive the message the server sends
    char buf[MAX_DATASIZE];
    int numbytes;

    if ((numbytes = recv(sockfd, buf, sizeof(buf) - 1, 0)) == -1) {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    buf[numbytes] = '\0';

    printf("client: recieved %s\n", buf);
    close(sockfd);
    exit(EXIT_SUCCESS);
}
