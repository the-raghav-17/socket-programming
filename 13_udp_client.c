#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>


#define SERVPORT "4960"
#define MAX_MSG_LEN 100


void *
get_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in *) sa)->sin_addr);
}


int
main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: ./udp-client <hostname>\n");
        exit(EXIT_FAILURE);
    }

    struct addrinfo  hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;

    int status;
    if ((status = getaddrinfo(argv[1], SERVPORT, &hints, &res)) == -1) {
        fprintf(stderr, "client: getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    int sockfd;
    struct addrinfo *p;

    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: Couldn't create socket\n");
        exit(EXIT_FAILURE);
    }

    char their_addrstr[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, get_addr((struct sockaddr *)p->ai_addr),
              their_addrstr, sizeof(their_addrstr));

    // Once we've created the socket, we can send message to it via UDP way
    char msg[MAX_MSG_LEN];
    printf("Enter the message to send: ");
    fgets(msg, sizeof(msg), stdin);

    int numbytes;
    if ((numbytes = sendto(sockfd, msg, sizeof(msg) - 1, 0,
                           p->ai_addr, p->ai_addrlen)) == -1) {
        perror("client: sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("client: Successfully sent message to %s\n", their_addrstr);
}
