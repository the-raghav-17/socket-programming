/*
 * Getting IP address of google via gethostbyname
 * which is a deprecated function
 */

#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>


int
main(void)
{
    struct hostent *ht = gethostbyname("google.com");
    if (ht == NULL) {
        perror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    char addr_str[INET6_ADDRSTRLEN];
    inet_ntop(ht->h_addrtype, (ht->h_addr_list)[0],
              addr_str, sizeof(addr_str));

    printf("%s\n", addr_str);

    exit(EXIT_SUCCESS);
}
