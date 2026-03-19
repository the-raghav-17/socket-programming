#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>


#define PORT "4960"
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
main(void)
{
    struct addrinfo  hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;

    int status;
    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) == -1) {
        fprintf(stderr, "server: getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    int sockfd;
    struct addrinfo *p;

    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: bind");
            close(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: Binding failed\n");
        exit(EXIT_FAILURE);
    }

    printf("server: listening for messages...\n");

    // It's a udp server, therefore no need to set up a listening and accepting
    // just make a recvfrom system call
    struct sockaddr_storage their_addr;
    socklen_t their_addrlen = sizeof(their_addr);
    char msg[MAX_MSG_LEN];
    int numbytes;

    if ((numbytes = recvfrom(sockfd, msg, sizeof(msg), 0,
         (struct sockaddr *)&their_addr, &their_addrlen)) == -1) {
        perror("client: recvfrom");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    char their_addrstr[INET6_ADDRSTRLEN];
    inet_ntop(their_addr.ss_family, get_addr((struct sockaddr *)&their_addr),
              their_addrstr, sizeof(their_addrstr));

    printf("server: Message recieved from %s\n", their_addrstr);
    printf("server: Message: %s\n", msg);
}
