/*
 * Name - Debanjan Saha
 * Roll - 19CS30014
 * Assignment - 2(b)
 * Description - DNS Server
 * Networks_Lab
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>

// For using gethostbyname() and the special data structure to describe the host
#include <netdb.h>

#define PORT 8181
#define BACKLOG 10
#define MAX_BUF_SIZE 100
#define MAX_IP_ADDR_LIST_LEN 1000

int max(int a, int b) {
	if (a > b) {
		return a;
	}
	return b;
}

void sigchld_handler(int s)
{
	while (wait(NULL) > 0);
}

int main(int argc, char const *argv[])
{
	int tcp_sockfd, udp_sockfd;
	struct sockaddr_in server_address, client_address;
	socklen_t addrlen;
	int bytes_read;

	struct hostent *host_info;

	char buffer[MAX_BUF_SIZE] = {0};


	/*
	 * +--------------------+
	 * |    TCP Setup       |
	 * +--------------------+
	 * Create the server side TCP socket file descriptor
	*/
	tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_sockfd == -1) {
		perror("TCP Socket creation failed");
		exit(EXIT_FAILURE);
	}

	/*
	 * Forcefully attaching TCP socket to the port 8181
	*/
	int force = 1;
	if (setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &force, sizeof(force)) == -1) {
		perror("Unable to forcefully attach TCP Socket to the PORT");
		exit(EXIT_FAILURE);
	}

	/*
	 * Bind the TCP socket to the port 8181
	*/
	if (bind(tcp_sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("TCP Socket Bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(tcp_sockfd, BACKLOG) < 0)
	{
		perror("Error while listening for incoming connections");
		exit(EXIT_FAILURE);
	}

	memset(&server_address, 0, sizeof(server_address));
	memset(&client_address, 0, sizeof(client_address));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons( PORT );

	/*
	 * +--------------------+
	 * |     UDP Setup      |
	 * +--------------------+
	 * Create the server side UDP socket file descriptor
	*/
	udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_sockfd == -1) {
		perror("UDP Socket creation failed");
		exit(EXIT_FAILURE);
	}

	/*
	 * Bind the UDP socket to the port 8181
	*/
	if (bind(udp_sockfd, (const struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("UDP Socket Bind failed");
		exit(EXIT_FAILURE);
	}



	struct sigaction sa;

	/*
	 * According to Beej's guide,
	 * The code that’s there is responsible for reaping zombie processes that appear as the fork()ed child processes exit.
	 * If you make lots of zombies and don’t reap them, your system administrator will become agitated.
	*/
	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	fd_set readfds;
	int numfd_max, ready_fd;

	// clear the set ahead of time
	FD_ZERO(&readfds);


	/*
	 * +--------------------+
	 * | PreSetup Complete  |
	 * +--------------------+
	*/
	printf("\n\
    +-------------------------------------+\n\
    |<<< DNS Server is up and running  >>>|\n\
    |<<<                               >>>|\n\
    |<<<     Press Ctrl + C to stop    >>>|\n\
    +-------------------------------------+\n\n");
	while (1) {

		// add our descriptors to the set
		FD_SET(tcp_sockfd, &readfds);
		FD_SET(udp_sockfd, &readfds);

		// get the "greater" fd, so we use that + 1 for the n param in select()
		numfd_max = max(tcp_sockfd, udp_sockfd) + 1;

		// select socket that has data ready to be recv()d
		ready_fd = select(numfd_max, &readfds, NULL, NULL, NULL);

		if (ready_fd == -1) {
			perror("Error occured in select");
			continue;
		}

		// printf("Waiting for a client...\n\n");

		/*
		 * +----------------------------------+
		 * | Check if TCP socket is readable  |
		 * +----------------------------------+
		*/
		if (FD_ISSET(tcp_sockfd, &readfds)) {
			addrlen = sizeof(client_address);
			int client_fd = accept(tcp_sockfd, (struct sockaddr *)& client_address, (socklen_t*)& addrlen);
			if (client_fd < 0) {
				perror("Failed to Accept TCP Connection");
				continue;
			}

			bytes_read = recv( client_fd , buffer, MAX_BUF_SIZE, 0);
			if (bytes_read == -1) {
				printf("Error during Receiving from Client. Please resend!\n");
				continue;
			}
			buffer[bytes_read] = '\0';

			pid_t pid = fork();
			if ( pid == 0 ) {
				// printf("Inside child\n");
				// inside child process

				//child will close listening socket
				close(tcp_sockfd);

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

				bytes_read = send(client_fd , ip_addresses , strlen(ip_addresses) , 0);
				printf("IP Address has been sent back!\n\n");
				// child terminates
				// printf("child exiting\n");
				exit(0);
			}
		}

		/*
		 * +----------------------------------+
		 * | Check if UDP socket is readable  |
		 * +----------------------------------+
		*/
		if (FD_ISSET(udp_sockfd, &readfds)) {

			addrlen = sizeof(client_address);

			bytes_read = recvfrom( udp_sockfd , buffer, MAX_BUF_SIZE, 0, (struct sockaddr *) &client_address, &addrlen);
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

			bytes_read = sendto(udp_sockfd , ip_addresses, strlen(ip_addresses) , 0, (const struct sockaddr *) &client_address, sizeof(client_address) );
			printf("IP Address has been sent back!\n\n");
		}


	}

	return 0;
}