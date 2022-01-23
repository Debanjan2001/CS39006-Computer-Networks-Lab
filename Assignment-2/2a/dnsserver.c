/*
 * Name - Debanjan Saha
 * Roll - 19CS30014
 * Assignment - 2(a)
 * Description - DNS Server
 * Networks_Lab
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

// For using gethostbyname() and the special data structure to describe the host
#include <netdb.h>

#define PORT 8080
#define MAX_BUF_SIZE 100
#define MAX_IP_ADDR_LIST_LEN 1000

int main(int argc, char const *argv[])
{
	int sockfd;
	struct sockaddr_in server_address, client_address;
	socklen_t addrlen = sizeof(client_address);
	int bytes_read;

	struct hostent *host_info;

	char buffer[MAX_BUF_SIZE] = {0};

	/*
	 * Create the server side socket file descriptor
	*/
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&server_address, 0, sizeof(server_address));
	memset(&client_address, 0, sizeof(client_address));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons( PORT );

	/*
	 * Bind the socket to the port 8181
	*/
	if (bind(sockfd, (const struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}

	printf("\n\
    +-------------------------------------+\n\
    |<<< DNS Server is up and running  >>>|\n\
    |<<<                               >>>|\n\
    |<<<     Press Ctrl + C to stop    >>>|\n\
    +-------------------------------------+\n\n");

	while (1) {
		// printf("Waiting for a client...\n\n");
		bytes_read = recvfrom( sockfd , buffer, MAX_BUF_SIZE, 0, (struct sockaddr *) &client_address, &addrlen);
		if (bytes_read == -1) {
			printf("Error during Receiving from Client. Please resend!\n");
			continue;
		}
		buffer[bytes_read] = '\0';

		host_info = gethostbyname(buffer);
		char ip_addresses[MAX_IP_ADDR_LIST_LEN];
		if ( host_info == NULL ) {
			// herror("gethostbyname failed");
			// continue;
			strcpy(ip_addresses, "0.0.0.0");
		} else {
			// success
			struct in_addr **addr_list = (struct in_addr **)host_info->h_addr_list;
			strcpy(ip_addresses, inet_ntoa(*addr_list[0]));

			for (int i = 1; addr_list[i] != NULL; i++) {
				strcat(ip_addresses, ", ");
				strcat(ip_addresses, inet_ntoa(*addr_list[i]));
			}
		}

		bytes_read = sendto(sockfd , ip_addresses, strlen(ip_addresses) , 0, (const struct sockaddr *) &client_address, sizeof(client_address) );
		printf("IP Address(es) has been sent back!\n\n");
	}

	return 0;
}