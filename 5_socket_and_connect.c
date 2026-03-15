#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * connect system call will associate the socket of the program with
 * a remote connection. connect is used by client applications that want
 * to communicate with a remote server. The remote server should bind its
 * socket with the IP address that it wants others to communicate it with.
 */


int
main(int argc, char *argv[])
{
    struct addrinfo  hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    // We won't set ai_flags to passive as we want to connect to a remote IP

    int status;
    if ((status = getaddrinfo("www.example.com", "3490", &hints, &res)) == -1) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect error");
        exit(EXIT_FAILURE);
    }

    //...
    // Do the work
    //...
}
