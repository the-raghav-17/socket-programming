#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


#define PORT "4950"
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
        perror("listener: getaddrinfo");
        exit(EXIT_FAILURE);
    }

    struct addrinfo *p;
    int sockfd;
    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("listener: bind");
            close(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: Binding failed\n");
        exit(EXIT_FAILURE);
    }

    printf("listener: waiting for someone to send data\n");
    // Now we've successfully binded to a listening socket, we'll recieve data from it
    char msg[MAX_MSG_LEN];
    int numbytes;

    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof(their_addr);

    if ((numbytes = recvfrom(sockfd, msg, sizeof(msg), 0,
                             (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("client: recvfrom");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // After recieving data from client, print client details and print the message
    char their_ip[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, (struct sockaddr *)&their_addr,
              their_ip, sizeof(their_ip));

    printf("listener: recieved data from %s\n", their_ip);
    printf("listener: packet is %d long\n", numbytes);

    msg[numbytes] = '\0';
    printf("listener: packet contents: \"%s\"\n", msg);

    close(sockfd);
    exit(EXIT_SUCCESS);
}
