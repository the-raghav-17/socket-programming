#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>


int
main(void)
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval tv = {2, 500000};  // 2.5s

    select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

    if (FD_ISSET(STDIN_FILENO, &readfds)) {
        printf("A key was pressed\n");
    } else {
        printf("Timeout\n");
    }

    exit(EXIT_SUCCESS);
}
