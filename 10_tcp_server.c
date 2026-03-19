#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>


#define PORT "3490"
#define BACKLOG 10
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
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    int status;
    if((status = getaddrinfo(NULL, PORT, &hints, &res)) == -1) {
        fprintf(stderr, "server: getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    int sockfd;
    int yes = 1;
    struct addrinfo *p;

    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("server: setsockopt");
            exit(EXIT_FAILURE);
        }

        if (bind(sockfd, (struct sockaddr *)p->ai_addr, p->ai_addrlen) == -1) {
            perror("server: bind");
            close(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: Binding to socket failed\n");
        exit(EXIT_FAILURE);
    }

    // At this point, we've successfully binded a socket. Now set it for listening
    if (listen(sockfd, BACKLOG) == -1) {
        perror("server: listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("server: listening for connections...\n");

    // Now accept connections
    struct sockaddr_storage their_addr;
    socklen_t their_addrlen = sizeof(their_addr);

    int new_fd;
    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &their_addrlen)) == -1) {
        perror("server: accept");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    close(sockfd);

    // We've successfully established a connection.
    // Print connection IP address and then send message
    char their_addrstr[INET6_ADDRSTRLEN];
    inet_ntop(their_addr.ss_family, get_addr((struct sockaddr *)&their_addr),
              their_addrstr, sizeof(their_addrstr));

    printf("server: Successfully connected with %s\n", their_addrstr);

    // Once successfully connected to a host, send message
    char msg[MAX_MSG_LEN];
    printf("Enter the message to send: ");
    fgets(msg, sizeof(msg), stdin);

    int numbytes;
    if ((numbytes = send(new_fd, msg, sizeof(msg) - 1, 0)) == -1) {
        perror("server: send");
        close(new_fd);
        exit(EXIT_FAILURE);
    }

    close(new_fd);
    exit(EXIT_SUCCESS);
}
