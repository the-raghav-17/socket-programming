#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>


#define SERVER_PORT "4950"


int
main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: ./datagram-client servername message\n");
        exit(EXIT_FAILURE);
    }

    struct addrinfo  hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;

    int status;
    if ((status = getaddrinfo(argv[1], SERVER_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    // Now traverse the list and create a socket
    struct addrinfo *p;
    int sockfd;

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: Can't create socket\n");
        exit(EXIT_FAILURE);
    }

    // socket is created at this point now

    int numbytes;
    if ((numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0,
                           p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(servinfo);

    printf("talker: send %d bytes to %s\n", numbytes, argv[1]);

    close(sockfd);
    exit(EXIT_FAILURE);
}
