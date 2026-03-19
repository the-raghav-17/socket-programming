#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>


#define SERVPORT "3490"
#define MAX_MSG_LEN 100


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
        fprintf(stderr, "usage: ./tcp-client <hostname>\n");
        exit(EXIT_FAILURE);
    }

    struct addrinfo  hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status;
    if ((status = getaddrinfo(argv[1], SERVPORT, &hints, &res)) == -1) {
        fprintf(stderr, "client: getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_SUCCESS);
    }

    int sockfd;
    struct addrinfo *p;

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

    if (p == NULL) {
        fprintf(stderr, "client: connection failed\n");
        exit(EXIT_FAILURE);
    }

    // get the address information
    char their_addr[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, get_addr((struct sockaddr *)p->ai_addr),
              their_addr, sizeof(their_addr));

    printf("client: Successfully connected to %s\n", their_addr);


    char msg[MAX_MSG_LEN];
    int numbytes;
    if ((numbytes = recv(sockfd, msg, sizeof(msg) - 1, 0)) == -1) {
        perror("client: recv");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    msg[numbytes] = '\0';

    printf("client: Message recieved: %s", msg);

    close(sockfd);
    exit(EXIT_SUCCESS);
}
