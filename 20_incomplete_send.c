/*
 * A client program that tries to send a huge chunk of data
 * to stream server.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define PORT "3490"
#define MAX_MSG_LEN 1000000


void *
get_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


int
get_socket(void)
{
    struct addrinfo  hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int rv;
    if ((rv = getaddrinfo("fedora", PORT, &hints, &res)) == -1) {
        fprintf(stderr, "client: getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    struct addrinfo *p;
    int sockfd;
    struct sockaddr_storage server_addr;
    socklen_t server_addrlen = sizeof(server_addr);

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
        fprintf(stderr, "client: failed to connect\n");
        return -1;
    }

    char server_addrstr[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family,
              get_addr((struct sockaddr *)&p->ai_addr),
              server_addrstr, sizeof(server_addrstr));

    printf("client: successfully connected to %s\n",
           server_addrstr);

    return sockfd;
}


void
sendall(int s, char *buf, size_t *len)
{
    int total = 0;           // total bytes sent till now
    int bytes_left = *len;  // total bytes left to send

    while (total < *len) {
        int n = send(s, buf + total, bytes_left, 0);
        if (n == -1) {
            break;
        }
        total += n;
        bytes_left -= n;
    }

    *len = total;
}


int
main(void)
{
    int sockfd = get_socket();
    if (sockfd == -1) {
        fprintf(stderr, "client: can't get socket\n");
        exit(EXIT_FAILURE);
    }

    printf("client: As we're now successfully connected to server, "
          "we'll try to send some huge chunk of data to it.\n");

    char *msg = NULL;
    size_t n  = 0;

    getline(&msg, &n, stdin);

    sendall(sockfd, msg, &n);

    exit(EXIT_SUCCESS);
}
