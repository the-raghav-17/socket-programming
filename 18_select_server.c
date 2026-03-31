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
#define MAX_MSG_LEN 100


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
    FD_ZERO(&fd_list->fl_fdset);
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

    fds[count]             = fd;
    count                 += 1;
    fd_list->fl_fdcount    = count;
    fd_list->fl_fdcapacity = capacity;
    fd_list->fl_fds        = fds;
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


void
remove_from_array(int i , Fd_list *fd_list)
{
    int fd = fd_list->fl_fds[i];
    FD_CLR(fd, &fd_list->fl_fdset);

    for (int j = i + 1; j < fd_list->fl_fdcount; i++) {
        fd_list->fl_fds[j - 1] = fd_list->fl_fds[j];
    }
}


void
remove_from_list(int fd, Fd_list *fd_list)
{
    for (int i = 0; i < fd_list->fl_fdcount; i++) {
        if (fd == fd_list->fl_fds[i]) {
            remove_from_array(i, fd_list);
        }
    }
}


int
get_max_fd(Fd_list *fd_list)
{
    if (fd_list->fl_fdcount == 0) {
        return -1;
    }

    int max_fd = fd_list->fl_fds[0];
    for (int i = 1; i < fd_list->fl_fdcount; i++) {
        int fd = fd_list->fl_fds[i];
        if (fd > max_fd) {
            max_fd = fd;
        }
    }

    return max_fd;
}


int *
get_changed_fds(Fd_list *fd_list, int num_fds)
{
    int *changed_fds = malloc(num_fds * sizeof(*changed_fds));
    int i = 0;

    for (int j = 0; j < fd_list->fl_fdcount; j++) {
        int fd = fd_list->fl_fds[j];
        if (FD_ISSET(fd, &fd_list->fl_fdset)) {
            changed_fds[i] = fd;
            i++;
        }
    }

    return changed_fds;
}


/* ========== Helper functions for the server ========== */


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


void
handle_listener_event(Fd_list *fd_list, int listener)
{
    struct sockaddr_storage remote_addr;
    socklen_t remote_addrlen = sizeof(remote_addr);

    int new_fd = accept(listener, (struct sockaddr *)&remote_addr,
                        &remote_addrlen);

    char remote_addrstr[INET6_ADDRSTRLEN];
    inet_ntop(remote_addr.ss_family, get_addr((struct sockaddr *)&remote_addr),
              remote_addrstr, sizeof(remote_addrstr));

    printf("server: Got connection from %s on socket %d\n",
           remote_addrstr, new_fd);

    add_to_list(new_fd, fd_list);
    FD_SET(listener, &fd_list->fl_fdset);
}


/* Broadcast message to everyone except sockfd1 and sockfd2 */
void
broadcast_msg(char *msg, int msg_len, Fd_list *fd_list,
              int sockfd1, int sockfd2)
{
    for (int i = 0; i < fd_list->fl_fdcount; i++) {
        int fd = fd_list->fl_fds[i];
        if (fd != sockfd1 && fd != sockfd2) {
            if (send(fd, msg, msg_len, 0) == -1) {
                perror("server: send");
            }
        }
    }
}


void
handle_client_event(Fd_list *fd_list, int listener, int client)
{
    /* Client has either sent a message or has hung up */
    char msg[MAX_MSG_LEN];
    int num_bytes = recv(client, msg, sizeof(msg), 0);

    if (num_bytes <= 0) {
        if (num_bytes == 0) {
            /* Client has hung up */
            printf("server: client on socket %d hung up\n",
                   client);
        }
        else {
            perror("server: recv");
        }
        remove_from_list(client, fd_list);
    }

    /* Client sent a message */
    broadcast_msg(msg, sizeof(msg), fd_list, listener, client);

    /* select modified the set. Put the fd in it */
    FD_SET(client, &fd_list->fl_fdset);
}


void
handle_event(Fd_list *fd_list, int num_fds, int listener)
{
    int *changed_fds = get_changed_fds(fd_list, num_fds);
    for (int i = 0; i < num_fds; i++) {
        int fd = changed_fds[i];
        if (fd == listener) {
            handle_listener_event(fd_list, listener);
        }
        else {
            handle_client_event(fd_list, listener, fd);
        }
    }

    free(changed_fds);
}


int
main(void)
{
    int listener = get_listener_socket();
    if (listener == -1) {
        fprintf(stderr, "server: failed to get listener socket\n");
        exit(EXIT_FAILURE);
    }

    Fd_list *fd_list = get_fd_list();
    add_to_list(listener, fd_list);

    printf("server: waiting for connections...\n");

    for (;;) {
        int max_fd  = get_max_fd(fd_list);
        int num_fds = select(max_fd + 1, &fd_list->fl_fdset,
                             NULL, NULL, NULL);
        if (num_fds == -1) {
            perror("server: select");
            continue;
        }

        handle_event(fd_list, num_fds, listener);
    }

    exit(EXIT_SUCCESS);
}
