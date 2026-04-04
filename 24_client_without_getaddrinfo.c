#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>


void
fill_addr(struct sockaddr_in *sin, socklen_t addrlen)
{
    memset(sin, 0, addrlen);
    sin->sin_family = AF_INET;
    sin->sin_port   = htons(3490);
    inet_pton(AF_INET, "127.0.0.1", &sin->sin_addr);
}


int
main(void)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr addr;
    socklen_t addrlen = sizeof(addr);
    fill_addr((struct sockaddr_in *)&addr, addrlen);

    if (connect(sockfd, &addr, addrlen) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Successfully connected\n");
    exit(EXIT_SUCCESS);
}
