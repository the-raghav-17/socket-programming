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
#define MAX_MSG_LEN 100


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


void
del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    pfds[i] = pfds[*fd_count - 1];
    (*fd_count)--;
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
        fprintf(stderr, "pollserver: failed to get listener socket\n");
        exit(EXIT_FAILURE);
    }

    int fd_size  = 5;
    int fd_count = 0;
    struct pollfd *pfds = malloc(fd_size * sizeof(*pfds));
    if (pfds == NULL) {
        perror("pollserver: malloc");
        exit(EXIT_FAILURE);
    }

    pfds[0].fd     = STDIN_FILENO;
    pfds[0].events = POLLIN;
    fd_count      += 1;

    for (;;) {
        int num_events = poll(pfds, fd_count, -1);
        if (num_events == -1) {
            perror("pollserver: poll");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < fd_count; i++) {
            if (pfds[i].revents & (POLLIN | POLLHUP)) {
                if (pfds[i].fd == listener) {
                    // Server recieved another connection request
                    struct sockaddr_storage remote_addr;
                    socklen_t remote_addrlen = sizeof(remote_addr);

                    int new_fd = accept(listener, (struct sockaddr *)&remote_addr,
                                        &remote_addrlen);
                    add_to_pfds(&pfds, new_fd, &fd_count, &fd_size);

                    char remote_addrstr[INET6_ADDRSTRLEN];
                    inet_ntop(remote_addr.ss_family,
                              get_addr((struct sockaddr *)&remote_addr),
                              remote_addrstr, sizeof(remote_addrstr));

                    printf("pollserver: Connected with %s on socket %d\n",
                           remote_addrstr, new_fd);
                }
                else {
                   // Already connected client sent some data
                    char msg[MAX_MSG_LEN];
                    int send_fd  = pfds[i].fd;

                    int numbytes = recv(send_fd, msg, sizeof(msg), 0);
                    if (numbytes <= 0) {
                        if (numbytes == 0) {
                            printf("pollserver: socket %d hung up\n", send_fd);
                        } else {
                            perror("pollserver: recv");
                        }
                        close(send_fd);
                        del_from_pfds(pfds, i, &fd_count);
                        i--;
                    }
                    else {
                        // Client sent some data; send it to everyone else
                        for (int j = 0; j < fd_count; j++) {
                            int recv_fd = pfds[i].fd;

                            if (recv_fd != listener && recv_fd != send_fd) {
                                if (send(recv_fd, msg, numbytes, 0) == -1) {
                                    perror("pollserver: send");
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    exit(EXIT_SUCCESS);
}
