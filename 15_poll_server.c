#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>


#define PORT "9034"
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
add_to_pfds(struct pollfd **pfds, int new_fd, int *fd_count, int *fd_size)
{
    if (*fd_count >= *fd_size) {
        *fd_size += 5;

        struct pollfd *temp = realloc(*pfds, *fd_size * sizeof(*temp));
        if (temp == NULL) {
            perror("realloc");
            return;
        }

        *pfds = temp;
    }


    (*pfds)[*fd_count].fd      = new_fd;
    (*pfds)[*fd_count].events  = POLLIN;
    (*pfds)[*fd_count].revents = 0;
    *fd_count += 1;
}


int
get_listener_socket(void)
{
    struct addrinfo  hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    int status;
    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) == -1) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    int listener;
    int yes = 1;
    struct addrinfo *p;

    for (p = res; p != NULL; p = p->ai_next) {
        if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            perror("setsockopt");
            close(listener);
            return -1;
        }

        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
            perror("bind");
            close(listener);
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Couldn't bind to socket\n");
        return -1;
    }

    if (listen(listener, BACKLOG) == -1) {
        perror("listen");
        return -1;
    }

    return listener;
}


int
main(void)
{
    int listener = get_listener_socket();
    if (listener == -1) {
        perror("Falied to get listener socket");
        exit(EXIT_FAILURE);
    }

    // Setup the pollfd array
    int fd_count        = 0;
    int fd_size         = 5;
    struct pollfd *pfds = malloc(fd_size * sizeof(*pfds));
    if (pfds == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    pfds[0].fd     = listener;
    pfds[0].events = POLLIN;
    fd_count       = 1;

    for (;;) {
        int poll_events = poll(pfds, fd_count, -1);
        if (poll_events == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < fd_count; i++) {
            // If a socket recieved a change
            if (pfds[i].revents & (POLLIN | POLLHUP)) {
                if (pfds[i].fd == listener) {
                    // We got another connection
                    struct sockaddr_storage remote_addr;
                    socklen_t addrlen = sizeof(remote_addr);

                    int new_fd = accept(listener, (struct sockaddr *)&remote_addr,
                                        &addrlen);
                    if (new_fd == -1) {
                        perror("accept");
                    }
                    else {
                        add_to_pfds(&pfds, new_fd, &fd_count, &fd_size);

                        char addrstr[INET6_ADDRSTRLEN];
                        inet_ntop(remote_addr.ss_family,
                                  get_addr((struct sockaddr *)&remote_addr),
                                  addrstr, sizeof(addrstr));

                        printf("pollserver: Got connection from %s on socket %d\n",
                               addrstr, new_fd);
                    }
                }
                else {
                    // Remote we're connected to sent a message
                }
            }
            else {
                // TODO: Fill this

            }
        }
    }

    exit(EXIT_SUCCESS);
}
