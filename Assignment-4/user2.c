#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define PORT 50097
#define RCVPORT 50096

#include "rsocket.c"

int main() {

    int sockfd = r_socket(AF_INET, SOCK_MRP, 0);
    if(sockfd < 0) {
        perror("ERROR:: r_socket() ");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    
    int err = r_bind(sockfd, (struct sockaddr*)&addr, addrlen);
    if(err < 0) {
        perror("ERROR:: r_bind() ");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in dest_addr;
    socklen_t dest_len = sizeof(dest_addr);

    char buffer[4];
    memset(buffer, 0, sizeof(buffer));
    while(1) {
        memset(buffer, 0, sizeof(buffer));
        err = r_recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&dest_addr, &dest_len);
        printf("Recieved...");
        if(err < 0) {
            perror("ERROR :: r_recvfrom() ");
        }
        printf(" %s \n", buffer);

    }

    printf("Done.\n");
    
    return 0;
}