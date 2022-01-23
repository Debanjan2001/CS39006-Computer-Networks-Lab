/*
 * Name - Debanjan Saha
 * Roll - 19CS30014
 * Assignment - 2)(a)
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
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define MAX_BUF_SIZE 100
#define MAX_IP_ADDR_LIST_LEN 1000

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

	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		perror("Timeout Setup Error");
		exit(EXIT_FAILURE);
	}

	memset(&server_address, 0, sizeof(server_address));

	char buffer[MAX_BUF_SIZE] = {0};

	// Server information
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons( PORT );
	server_address.sin_addr.s_addr = INADDR_ANY;

	char domain_name[MAX_BUF_SIZE];
	printf(">> Enter a DNS Name: ");
	scanf("%s", domain_name);

	strcpy(buffer, domain_name);
	int bytes_read = 0;

	bytes_read = sendto(sockfd , buffer, strlen(buffer) , 0, (const struct sockaddr *) &server_address, sizeof(server_address) ) ;
	if (bytes_read == -1) {
		perror("Error while sending the DNS Name!");
	}
	printf("\nDNS Name Successfully Sent! :-)\n\n");

	char received_message[MAX_IP_ADDR_LIST_LEN];
	bytes_read = recvfrom( sockfd , received_message, MAX_IP_ADDR_LIST_LEN, 0, (struct sockaddr *) &server_address, &addrlen);
	if (bytes_read == -1) {
		printf("Waited for 2 seconds...\nNo Response From Server, Exiting!\n");
		exit(EXIT_FAILURE);
	}
	// Print the final message received from the server
	printf("IP Address(es) Received From Server.\n");
	printf("IP Address(es) : %s\n", received_message);

	close(sockfd);

	return 0;
}