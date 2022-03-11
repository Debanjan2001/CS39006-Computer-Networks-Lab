#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define PORT 50096
#define RCVPORT 50097

#include "rsocket.h"

int main() {

    int sockfd = r_socket(AF_INET, SOCK_MRP, 0);
    if(sockfd < 0) {
        perror("ERROR:: r_socket() ");
        exit(EXIT_FAILURE);
    }
    // printf("User1 Sockfd = %d\n",sockfd);

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

    char message[50];
    memset(message, 0, sizeof(message));
    printf("Enter a message [25-30 characters] :\n");
    scanf("%s", message);

    struct sockaddr_in dest_addr;
    socklen_t dest_len = sizeof(dest_addr);
    memset(&dest_addr, 0, sizeof(struct sockaddr_in));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = INADDR_ANY;
    dest_addr.sin_port = htons(RCVPORT);

    int msglen = strlen(message);
    char buffer[2];
    memset(buffer, 0, sizeof(buffer));
    for(int i = 0 ; i < msglen; i++) {
        buffer[0] = message[i];
        err = r_sendto(sockfd, buffer, 1, 0, (struct sockaddr *)&dest_addr, dest_len);
        if(err < 0) {
            perror("ERROR :: r_sendto() ");
        }
    }

    printf("Done.\n");
    while(1);
    return 0;
}