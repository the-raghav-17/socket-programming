/*
 * A simple server implementation that listens on port 3490.
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>


#define PORT    "3490"
#define BACKLOG 10


void *
get_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


void
sigchld_handler(int s)
{
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}


int
main(void)
{
    // first get the details required to create and bind a socket
    struct addrinfo  hints;  // acts as a template
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // use any of IPv4 and IPv6
    hints.ai_socktype = SOCK_STREAM; // tcp stream socket
    hints.ai_flags    = AI_PASSIVE; // we're the server thus fill our own IP address

    int status;
    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) == -1) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    // now we will traverse the resulting linked list and bind to the first socket
    int sockfd;
    struct addrinfo *p;
    int yes = 1;
    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("server: setsockopt");
            exit(EXIT_FAILURE);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("server: bind");
            close(sockfd);
            continue;
        }

        break;
    }

    freeaddrinfo(res); // no longer need this

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(EXIT_FAILURE);
    }

    // now we've successfully binded and have a socket ready for listening
    // BACKLOG is the number of connections that can queue
    if (listen(sockfd, BACKLOG) == -1) {
        perror("server: listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("server: waiting for connections...\n");

    // now in an infinite loop, the server will accept the connections available
    // blocking the control

    // setup a handler that will reap terminated children
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags   = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    while (1) {
        struct sockaddr_storage their_addr; // large enough to hold both IPv4 and IPv6
        socklen_t addr_len = sizeof(their_addr);

        // new_fd is the socket for communicating with the connected machine
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_len);
        if (new_fd == -1) {
            perror("server: accept");
            continue;
        }

        // Get the connection details
        char s[INET6_ADDRSTRLEN]; // has enough space to hold IPv4/IPv6 address string
        inet_ntop(their_addr.ss_family, get_addr((struct sockaddr *)&their_addr),
                  s, sizeof(s));

        printf("server: got connection from %s\n", s);

        // Spawn a child process that will handle the connection
        if (fork() == 0) {
            close(sockfd);  // doesn't need this
            char msg[] = "Hello, world";
            if (send(new_fd, msg, sizeof(msg), 0) == -1) {
                perror("send");
            }
            close(new_fd);
            exit(EXIT_SUCCESS);
        }

        close(new_fd); // parent doesn't need this
    }

    exit(EXIT_SUCCESS);
}
