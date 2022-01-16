/*
 * Name - Debanjan Saha
 * Roll - 19CS30014
 * Assignment - 1
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

#define PORT 8080
#define MAX_BUF_SIZE 100

// You can change the size of the chunk used by client for sending the file
#define CHUNK_SIZE 50


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

	char buffer[CHUNK_SIZE] = {0};

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);

	// Convert IPv4 and IPv6 addresses from text to binary form
	if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	/*
	 * Searching for a filename in the arguments
	*/
	if (argc < 2) {
		printf("No filename given!\n");
		exit(EXIT_FAILURE);
	}
	const char *filename = argv[1];

	int fd = open(filename, O_RDONLY);

	if (fd < 0) {
		printf("File Not Found!\n");
		exit(EXIT_FAILURE);
	}

	/*
	 * Connect the client socket to the server socket
	*/
	if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		printf("\nConnection Failed \n");
		return -1;
	}

    printf("\nConnected to Server on port %d...\n\n",PORT);

	/*
	 * Sending the file in multiple chunks => inside the for loop
	*/
	while (1) {

		int bytes_read = 0;
		char received_message[MAX_BUF_SIZE];

		printf("Sending file chunks by chunks...\n");

		for (;;) {
			bytes_read = read(fd, buffer, CHUNK_SIZE);
			if (bytes_read > 0) {
				buffer[bytes_read] = '\0';
				send(sockfd , buffer, strlen(buffer) , 0 );
				bytes_read = recv( sockfd , received_message, MAX_BUF_SIZE, 0);
				received_message[bytes_read] = '\0';
				if (bytes_read <= 0) {
					break;
				}
				// printf("%ld, %s\n",strlen(received_message), buffer);
			} else {
				// no more chunks to read. hence terminate
				break;
			}
		}

		printf("File Transfer Complete...\n\n");

		// Print the final message received from the server
		printf("Message Received From Server.\n");
		printf("[MESSAGE] %s\n", received_message);
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