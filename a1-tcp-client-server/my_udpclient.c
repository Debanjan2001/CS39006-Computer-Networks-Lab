/*
 * Name - Debanjan Saha
 * Roll - 19CS30014
 * Assignment - 1
 * Description - UDP Client 
 * Networks_Lab
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <errno.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define PORT 8181
#define MAX_BUF_SIZE 100

// You can change the size of the chunk used by client for sending the file
#define CHUNK_SIZE 50

int main(int argc, char const *argv[])
{
	int sockfd = 0;
	struct sockaddr_in server_address;
    socklen_t addrlen = sizeof(server_address);
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
	{
		printf("Socket creation error\n");
		exit(1);
	}

    memset(&server_address, 0, sizeof(server_address)); 

	char buffer[CHUNK_SIZE] = {0};

	// Server information 
    server_address.sin_family = AF_INET; 
    server_address.sin_port = htons( PORT ); 
    server_address.sin_addr.s_addr = INADDR_ANY; 
	
    if(argc < 2){
        printf("No filename given!\n");
        exit(EXIT_FAILURE);
    }
    const char *filename = argv[1];

    int fd = open(filename, O_RDONLY);

    if(fd < 0){
        printf("File Not Found!\n");
        exit(EXIT_FAILURE);
    }


    while(1){

        int bytes_read = 0;
        char received_message[MAX_BUF_SIZE];

        printf("\nPreparing to send file in chunks...\n");
       
        for(;;){
            bytes_read = read(fd, buffer, CHUNK_SIZE);
            // printf("%d\n",bytes_read);
            if(bytes_read > 0){
                buffer[bytes_read] = '\0';
                int val_read = sendto(sockfd , buffer, strlen(buffer) , 0, (const struct sockaddr *) &server_address, sizeof(server_address) );
                // printf("%d\n",val_read);
                // if(val_read < 0){
                //     perror("Error while sending\n");
                // }
                // printf("%ld, %s\n",strlen(received_message), buffer);
            }else{
                /*
                 * Send an empty string to denote file transfer has completed.
                */
                printf("File sent completely.\n\n");
                bzero(buffer,sizeof(buffer));
                int val_read = sendto(sockfd , buffer, strlen(buffer) , 0, (const struct sockaddr *) &server_address, sizeof(server_address) );
                bytes_read = recvfrom( sockfd , received_message, MAX_BUF_SIZE, 0, (struct sockaddr *) &server_address, &addrlen);
                received_message[bytes_read] = '\0';
                break;
            }
        }

        // Print the final message received from the server
		printf("Message Received From Server.\n");
		printf("[MESSAGE] %s\n", received_message);

        break;
    }

    close(fd);
    close(sockfd);

	return 0;
}