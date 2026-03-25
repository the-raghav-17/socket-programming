/*
 * Trying to implement a server that doesn't use poll system call
 * and still able to manage multiple connections
 */


#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>


#define PORT "9034"
#define BACKLOG 10
#define MAX_MSG_LEN 100


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
        fprintf(stderr, "pollserver: getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    int listener;
    int yes = 1;
    struct addrinfo *p;

    for (p = res; p != NULL; p = p->ai_next) {
        if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("pollserver: socket");
            continue;
        }

        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            perror("pollserver: setsockopt");
            close(listener);
            return -1;
        }

        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
            perror("pollserver: bind");
            close(listener);
            continue;
        }
        break;
    }

    if (p == NULL) {
        perror("pollserver: binding failed\n");
        return -1;
    }

    /* Dont block if no connections are available */
    fcntl(listener, F_SETFL, O_NONBLOCK);

    if (listen(listener, BACKLOG) == -1) {
        perror("pollserver: listen");
        return -1;
    }
    return listener;
}


void
add_to_pfds(int **pfds, int new_fd,
            int *fd_count, int *fd_size)
{
    if (*fd_size <= *fd_count) {
        *fd_size *= 2;
        *pfds = realloc(*pfds, *fd_size * sizeof(**pfds));
    }

    fcntl(new_fd, F_SETFL, O_NONBLOCK);

    (*pfds)[*fd_count] = new_fd;
    *fd_count += 1;
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
handle_listener_event(int **pfds, int listener,
                      int *fd_count, int *fd_size)
{
    struct sockaddr_storage remote_addr;
    socklen_t remote_addrlen = sizeof(remote_addr);

    int new_fd = accept(listener, (struct sockaddr *)&remote_addr,
                        &remote_addrlen);
    if (new_fd == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("pollserver: accept");
        }
        return;
    }

    add_to_pfds(pfds, new_fd, fd_count, fd_size);

    char remote_addrstr[INET6_ADDRSTRLEN];
    inet_ntop(remote_addr.ss_family, get_addr((struct sockaddr *)&remote_addr),
              remote_addrstr, sizeof(remote_addrstr));

    printf("pollserver: connected to %s on socket %d\n",
           remote_addrstr, new_fd);
}


void
delete_from_pfds(int **pfds, int i, int *fd_count)
{
    for (int j = i + 1; j < *fd_count; j++) {
        (*pfds)[j - 1] = (*pfds)[j];
    }

    *fd_count -= 1;
}


void
handle_client_event(int **pfds, int i, int listener,
                    int *fd_count, int *fd_size)
{
    int sender = (*pfds)[i];
    char msg[MAX_MSG_LEN];

    int num_bytes = recv(sender, msg, sizeof(msg), 0);
    if (num_bytes <= 0) {
        if (num_bytes == 0) {
            printf("pollserver: %d disconnected\n", sender);
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("pollserver: recv");
            }
            /* No message to read */
            return;
        }
        delete_from_pfds(pfds, i, fd_count);
        i--;
    }
    else {
        // Send the message to everyone else {
        for (int j = 0; j < *fd_count; j++) {
            int reciever = (*pfds)[j];
            if (reciever != listener && reciever != sender) {
                if (send(reciever, msg, sizeof(msg), 0) == -1) {
                    perror("pollserver: send");
                }
            }
        }
    }
}


int
main(void)
{
    int fd_count = 0;    // count of fd's to poll for
    int fd_size  = 1;    // size of pfds array
    int *pfds    = malloc(fd_size * sizeof(*pfds));

    int listener = get_listener_socket();
    if (listener == -1) {
        fprintf(stderr, "pollserver: failed to get listener socket\n");
        exit(EXIT_FAILURE);
    }

    pfds[0]   = listener;
    fd_count += 1;

    printf("pollserver: waiting for connections...\n");

    for (;;) {
        for (int i = 0; i < fd_count; i++) {
            if (pfds[i] == listener) {
                handle_listener_event(&pfds, listener,
                      &fd_count, &fd_size);
            } else {
                handle_client_event(&pfds, i, listener,
                                    &fd_count, &fd_size);
            }
        }
    }

    exit(EXIT_SUCCESS);
}
