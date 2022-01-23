/*
 * Name - Debanjan Saha
 * Roll - 19CS30014
 * Assignment - 2)(b)
 * Description - TCP Client
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
#define MAX_BUF_SIZE 256
#define MAX_IP_ADDR_LIST_LEN 512

int main(int argc, char const *argv[])
{
	int sockfd = 0;
	struct sockaddr_in server_address;
	char buffer[MAX_BUF_SIZE] = {0};

	sockfd = socket(AF_INET, SOCK_STREAM , 0);
	if (sockfd < 0)
	{
		printf("Socket creation error\n");
		exit(1);
	}

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);

	// Convert IPv4 and IPv6 addresses from text to binary form
	if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	// Prompt User for DNS Name
    char domain_name[MAX_BUF_SIZE];
	printf(">> Enter a DNS Name: ");
	scanf("%s", domain_name);

	/*
	 * Connect the client socket to the server socket
	*/
	if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("Is the server up? connect() failed");
		return -1;
	}

    printf("\nConnected to Server on port %d...\n\n",PORT);


	strcpy(buffer, domain_name);
	int bytes_read = 0;
	
    bytes_read = send(sockfd , buffer, strlen(buffer) , 0 );
    if(bytes_read < 0){
        printf("Error during sending DNS Name. Please Retry!\n");
        exit(EXIT_FAILURE);
    }
    
    char received_message[MAX_IP_ADDR_LIST_LEN];

    bytes_read = recv( sockfd , received_message, MAX_IP_ADDR_LIST_LEN, 0);
    received_message[bytes_read] = '\0';

    if (bytes_read < 0) {
        printf("Error during Receiving IP Addresses. Please Retry!\n");
        exit(EXIT_FAILURE);
    }

    // Print the final message received from the server
    if(strcmp(received_message,"0.0.0.0") == 0){
		printf("No IP Address Resolved (0.0.0.0 Returned)\n");
	}else{
		printf("IP Address(es) Received From Server.\n");
		printf("IP Address(es) :%s\n", received_message);
	}

	close(sockfd);

	return 0;
}