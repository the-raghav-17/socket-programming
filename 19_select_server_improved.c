#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>


#define PORT          "3490"
#define BACKLOG        10
#define MAX_MSG_LEN    100


int
get_listener_socket(void)
{
    struct addrinfo  hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    int rv;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &res)) == -1) {
        fprintf(stderr, "server: getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    int yes = 1;
    int listener;
    struct addrinfo *p;
    for (p = res; p != NULL; p = p->ai_next) {
        if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
                       &yes, sizeof(yes)) == -1) {
            perror("server: setsockopt");
            close(listener);
            free(res);
            return -1;
        }

        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
            perror("server: bind");
            close(listener);
            continue;
        }
        break;
    }

    free(res);

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind socket\n");
        return -1;
    }

    if (listen(listener, BACKLOG) == -1) {
        perror("server: listen");
        close(listener);
        return -1;
    }

    return listener;
}


void *
get_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


void
handle_listener_socket(int listener, fd_set *master, int *maxfd)
{
    struct sockaddr_storage remote_addr;  // address of client
    socklen_t remote_addrlen = sizeof(remote_addr);

    int new_fd = accept(listener,
                        (struct sockaddr *) &remote_addr,
                        &remote_addrlen);

    if (new_fd == -1) {
        perror("server: accept");
    }
    else {
        // Add the client to master set
        FD_SET(new_fd, master);

        // Update the maxfd, if appropriate
        if (new_fd > *maxfd) {
            *maxfd = new_fd;
        }

        char remote_addrstr[INET6_ADDRSTRLEN]; // address string of client

        // Extract the address string
        inet_ntop(remote_addr.ss_family,
                get_addr((struct sockaddr *)&remote_addr),
                remote_addrstr, sizeof(remote_addrstr));

        printf("server: got connection from %s on socket %d\n",
            remote_addrstr, new_fd);
    }
}


void
broadcast(char *msg, int msg_len, int listener, int sender,
          fd_set *master, int *maxfd)
{
    // Send message to everyone, except the listener and the sender
    for (int i = 0; i <= *maxfd; i++) {
        if (FD_ISSET(i, master))
            if (i != listener && i != sender) {
                if (send(i, msg, msg_len, 0) == -1) {
                    perror("server: send");
                }
            }
    }
}


void
handle_client_socket(int listener, int sender, fd_set *master, int *maxfd)
{
    char msg[MAX_MSG_LEN];
    int nbytes;

    if ((nbytes = recv(sender, msg, sizeof(msg), 0)) <= 0) {
        if (nbytes == 0) {
            // client disconnected
            printf("server: %d hung up\n", sender);
        } else {
            perror("server: send");
        }

        // close connection to sender
        close(sender);
        FD_CLR(sender, master);
    } else {
        // Some data is recieved; broadcast it to everyone
        broadcast(msg, nbytes, listener, sender, master, maxfd);
    }
}


int
main(void)
{
    int listener = get_listener_socket();
    if (listener == -1) {
        fprintf(stderr, "server: failed to get listener socket\n");
        exit(EXIT_FAILURE);
    }

    fd_set master;  // keeps record of all sockets
    fd_set readfds;    // copy of the above; pass to select

    FD_ZERO(&master);
    FD_ZERO(&readfds);

    FD_SET(listener, &master);

    // we need to keep a record of the maximum fd.
    int maxfd = listener;

    printf("server: waiting for connections...\n");

    for (;;) {
        readfds = master;

        // Wait for change in any socket
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) == -1) {
            perror("server: select");
            exit(EXIT_FAILURE);
        }

        // Socket state change detected

        // find the socket whose state is changed
        for (int i = 0; i <= maxfd; i++) {
            if (FD_ISSET(i, &readfds)) {   // found it!
                if (i == listener) {
                    handle_listener_socket(listener, &master, &maxfd);
                }
                else {
                    handle_client_socket(listener, i, &master, &maxfd);
                }
            }
        }
    }

    exit(EXIT_SUCCESS);
}
