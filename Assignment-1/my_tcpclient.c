
// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <errno.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#define PORT 8080

#define BUFFER_SIZE 25
#define MAX_BUF_SIZE 100

int main(int argc, char const *argv[])
{
	int sockfd = 0;
	struct sockaddr_in server_address;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
	{
		printf("Socket creation error\n");
		exit(1);
	}

	char buffer[BUFFER_SIZE] = {0};

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	
	// Convert IPv4 and IPv6 addresses from text to binary form
	if(inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr)<=0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

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

    /*
     * connect the client socket to the server socket
    */
	if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    while(1){

        int bytes_read = 0;
        char received_message[MAX_BUF_SIZE];

        for(;;){
            bytes_read = read(fd, buffer, BUFFER_SIZE);
            if(bytes_read > 0){
                buffer[bytes_read] = '\0';
                send(sockfd , buffer, strlen(buffer) , 0 );
                int bytes_read = recv( sockfd , received_message, MAX_BUF_SIZE, 0);
                if(bytes_read <= 0){
                    break;
                }
                // received_message[strlen(received_message)] = '\0';
                // printf("%ld, %s\n",strlen(received_message), buffer);
            }else{
                break;
            }
        }
            
        printf("Message From Server: ");
        printf("%s\n",received_message);
        break;
    }
    
    close(fd);
    int status = shutdown(sockfd, O_RDWR);
    if (status < 0)
    {
        perror("Error during connection shutdown");
        exit(EXIT_FAILURE);
    }
    close(sockfd);
	
	return 0;
}