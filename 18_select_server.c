#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>


#define PORT "9034"
#define BACKLOG 10


/* ========== Fd_list object ========== */


/* List to monitor and update fds in fd_set */
typedef struct Fd_list
{
    fd_set fl_fdset;        /* set of fds */
    int   *fl_fds;          /* fds in fdset */
    int    fl_fdcount;      /* no. of fds in fdset */
    int    fl_fdcapacity;   /* size of fl_fds array */
    int    fl_maxfd;        // TODO: use it effectively
} Fd_list;


Fd_list *
get_fd_list(void)
{
    Fd_list *fd_list = malloc(sizeof(*fd_list));

    int capacity = 5;
    int *fds     = malloc(capacity * sizeof(*fds));

    /* Default values for attributes */
    fd_list->fl_fds        = fds;
    fd_list->fl_fdcapacity = capacity;
    fd_list->fl_fdcount    = 0;
    fd_list->fl_maxfd      = -1;

    return fd_list;
}


void
add_to_array(int fd, Fd_list *fd_list)
{
    int count    = fd_list->fl_fdcount;
    int capacity = fd_list->fl_fdcapacity;
    int *fds     = fd_list->fl_fds;

    if (capacity <= count) {
        capacity *= 2;
        fds       = realloc(fds, capacity * sizeof(*fds));
    }

    fds[count]           = fd;
    count               += 1;
    fd_list->fl_fdcount    = count;
    fd_list->fl_fdcapacity = capacity;
    fd_list->fl_fds      = fds;
}


void
add_to_list(int fd, Fd_list *fd_list)
{
    FD_SET(fd, &fd_list->fl_fdset);
    add_to_array(fd, fd_list);

    if (fd > fd_list->fl_maxfd) {
        fd_list->fl_maxfd = fd;
    }
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

    int rv;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &res)) == -1) {
        fprintf(stderr, "server: getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    int sockfd;
    int yes = 1;
    struct addrinfo *p;

    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            perror("server: setsockopt");
            close(sockfd);
            return -1;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("server: bind");
            close(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: binding failed\n");
        return -1;
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("server: listen");
        close(sockfd);
        return -1;
    }

    return sockfd;
}


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
    int listener = get_listener_socket();
    if (listener == -1) {
        fprintf(stderr, "server: failed to get listener socket\n");
        exit(EXIT_FAILURE);
    }

    int fd_size  = 1;    /* capacity of fds array */
    int fd_count = 0;    /* actual fd count in the array */
    int *fds     = malloc(fd_size * sizeof(*fds));
    fds[0]       = listener;
    fd_count    += 1;

    fd_set readfds;
    FD_SET(listener, &readfds);

    printf("server: waiting for connections...\n");

    for (;;) {
        int num_fds = select(fds[fd_count - 1] + 1, &readfds, NULL,
                             NULL, NULL);
        if (num_fds == -1) {
            perror("server: select");
            continue;
        }

        /* We've got a state change in one or more of the fds */
        for (int i = 0; i < fd_count; i++) {
            int fd = fds[i];
            if (FD_ISSET(fd, &readfds)) {
                // TODO: check if listener or client and handle appropriately
                if (fd == listener) {
                    /* New connection */
                    struct sockaddr_storage remote_addr;
                    socklen_t remote_addrlen = sizeof(remote_addr);
                    int new_fd = accept(listener, (struct sockaddr *)&remote_addr,
                                        &remote_addrlen);

                    char remote_addrstr[INET6_ADDRSTRLEN];
                    inet_ntop(remote_addr.ss_family, get_addr((struct sockaddr *)&remote_addr),
                              remote_addrstr, sizeof(remote_addrstr));
                    printf("server: got new connection from %s on socket %d\n",
                           remote_addrstr, new_fd);

                    /* add the new connection to set */
                    FD_SET(new_fd, &readfds);

                    if (fd_size <= fd_count) {
                        fd_size *= 2;
                        fds  = realloc(fds, fd_size * sizeof(*fds));
                    }
                    fds[fd_count] = new_fd;
                    fd_count += 1;
                }
                else {
                    /* client sent a message */
                }
            }
        }
    }

    exit(EXIT_FAILURE);
}
