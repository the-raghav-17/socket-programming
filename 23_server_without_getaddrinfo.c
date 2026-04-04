#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>


void *
get_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


void
fill_addr(struct sockaddr_in *sin, socklen_t len)
{
    memset(sin, 0, len);
    sin->sin_family = AF_INET;
    sin->sin_port   = htons(3490);
    inet_pton(AF_INET, "127.0.0.1", &sin->sin_addr);
}


int
main(void)
{
    /* Create a IPv6 TCP socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Once socket is created, bind it to local address */
    /*
     * To do so, we first need to fillout the struct sockaddr_in
     * with the details like PORT no and IP address
     */

    struct sockaddr addr;
    socklen_t addrlen = sizeof(addr);
    fill_addr((struct sockaddr_in *)&addr, addrlen);

    if (bind(sockfd, &addr, addrlen) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    printf("Successfully binded the socked\n");
    if (listen(sockfd, 10) == -1) {
        perror("listen");
    }

    accept(sockfd, NULL, NULL);
    printf("Connected to client\n");

    exit(EXIT_SUCCESS);
}
