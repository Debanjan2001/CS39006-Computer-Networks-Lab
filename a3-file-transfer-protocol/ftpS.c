/*
 * Name - Debanjan Saha + Pritkumar Godhani
 * Roll - 19CS30014 + 19CS10048
 * Assignment - 3
 * Description - File Transfer Using Sockets
 * Networks_Lab
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>

// For using gethostbyname() and the special data structure to describe the host
#include <netdb.h>

// Directory Ops
#include <dirent.h>

// chdir function is declared inside this header
#include<unistd.h>

// for file operations
#include<fcntl.h>

#define PORT 65532
#define MAX_BUF_SIZE 512
#define MAX_CMD_LEN 200
#define BACKLOG 15

const char* SUCCESSFUL_REQUEST = "200";
const char* UNSUCCESSFUL_REQUEST = "500";
const char* INVALID_REQUEST = "600";

#define HEADER_LEN 3


int min(int a, int b){
	if(a < b){
		return a;
	}
	return b;
}

int check_username(char *username) {
	// Return 0 if Username **NOT** Present /  File can not be opened
	// Return 1 if Username Present
	FILE * fp;
	fp = fopen("user.txt", "r");
	if (fp == NULL) {
		printf("user.txt file could not be opened!");
		return 0;
	}


	char line[MAX_BUF_SIZE];

	char file_username[MAX_BUF_SIZE];

	while (fgets(line, MAX_BUF_SIZE, fp)) {
		int len = strlen(line);
		int username_len = 0;
		for (int i = 0; i < len; i++) {
			if (line[i] == ' ') {
				break;
			}
			username_len += 1;
		}

		memcpy(file_username, &line[0], username_len);
		file_username[username_len] = '\0';

		if (strcmp(file_username, username) == 0) {
			fclose(fp);
			return 1;
		}
	}

	fclose(fp);
	return 0;
}

int check_password(char *username, char *password) {
	// Return 0 if Username **NOT** Present /  File can not be opened
	// Return 1 if Username Present
	FILE * fp;
	fp = fopen("user.txt", "r");
	if (fp == NULL) {
		printf("user.txt file could not be opened!");
		return 0;
	}

	char line[MAX_BUF_SIZE];

	char file_username[MAX_BUF_SIZE], file_password[MAX_BUF_SIZE];

	while (fgets(line, MAX_BUF_SIZE, fp)) {
		int len = strlen(line);

		int username_len = 0;
		for (int i = 0; i < len; i++) {
			if (line[i] == ' ') {
				break;
			}
			username_len += 1;
		}

		int password_len = 0;
		for (int i = username_len + 1; i < len; i++) {
			if (line[i] == '\0' || line[i] == '\n') {
				break;
			}
			password_len += 1;
		}

		memcpy(file_username, &line[0], username_len);
		file_username[username_len] = '\0';

		memcpy(file_password, &line[username_len + 1], password_len);
		file_password[password_len] = '\0';

		if (strcmp(file_username, username) == 0 && strcmp(file_password, password) == 0) {
			fclose(fp);
			return 1;
		}
	}

	fclose(fp);

	return 0;
}

int get_first_word_length(char *line) {
	int len = 0;
	for (int i = 0; i < strlen(line); i++) {
		if (line[i] == '\n' || line[i] == '\0' || line[i] == ' ') {
			break;
		}
		len += 1;
	}
	// printf("Len=%d\n",len);
	return len;
}

int main(int argc, char const *argv[]) {

	int tcp_sockfd;
	struct sockaddr_in server_address, client_address;
	socklen_t addrlen;
	int bytes_read;

	char buffer[MAX_BUF_SIZE];
	bzero(buffer, sizeof(buffer));

	memset(&server_address, 0, sizeof(server_address));
	memset(&client_address, 0, sizeof(client_address));

	// INADDR_ANY is to be changed into a dotted IP Address
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons( PORT );

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
	 * Forcefully attaching TCP socket to the defined PORT
	*/
	int force = 1;
	if (setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &force, sizeof(force)) == -1) {
		perror("Unable to forcefully attach TCP Socket to the PORT");
		exit(EXIT_FAILURE);
	}

	/*
	 * Bind the TCP socket to the defined PORT
	*/
	if (bind(tcp_sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
		perror("TCP Socket Bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(tcp_sockfd, BACKLOG) < 0) {
		perror("Error while listening for incoming connections");
		exit(EXIT_FAILURE);
	}

	/*
	 * +--------------------+
	 * | PreSetup Complete  |
	 * +--------------------+
	*/

	printf("\n\
    +-------------------------------------+\n\
    |<<<    Server is up and running   >>>|\n\
    |<<<  File Transfer using Sockets  >>>|\n\
    |<<<     Press Ctrl + C to stop    >>>|\n\
    +-------------------------------------+\n\n");

	while (1) {
		// printf("Waiting for a client...\n\n");

		addrlen = sizeof(client_address);
		int client_fd = accept(tcp_sockfd, (struct sockaddr *)& client_address, (socklen_t*)& addrlen);
		if (client_fd < 0) {
			perror("Failed to Accept TCP Connection");
			continue;
		}
		printf("Connection Accepted For a Client!\n");


		pid_t pid = fork();
		if ( pid == 0 ) {
			// printf("Inside child\n");
			// inside child process

			//child will close listening socket
			close(tcp_sockfd);

			// Operation string will be stored here
			char command[10];

			char current_directory[MAX_BUF_SIZE];
			getcwd(current_directory, sizeof(current_directory));
			// printf("dir:: %s\n", current_directory);

			/*
			* +--------------------+
			* |  User-Auth Check   |
			* +--------------------+
			*/
			for (;;) {
				int command_len = 4;

				// Wait for 'user <username>'
				bzero(buffer, sizeof(buffer));
				bytes_read = recv( client_fd , buffer, MAX_CMD_LEN, 0);
				if (bytes_read == -1) {
					printf("Error during Receiving from Client. Please Reconnect!\n");
					break;
				} else if (bytes_read == 0) {
					//connection broken
					break;
				}
				buffer[bytes_read] = '\0';

				memcpy(command, &buffer[0], command_len);
				command[command_len] = '\0';


				char username[MAX_BUF_SIZE], password[MAX_BUF_SIZE];

				if (strcmp(command, "user") == 0) {

					int username_len = strlen(buffer) - command_len - 1;
					memcpy(username, &buffer[command_len + 1], username_len);
					username[username_len] = '\0';

					// Check if username actually exists
					if (check_username(username)) {
						// exists
						bzero(buffer, sizeof(buffer));
						strcpy(buffer, SUCCESSFUL_REQUEST);
						bytes_read = send(client_fd , buffer , strlen(buffer) , 0);
						// success means move forward and wait for password
					} else {
						// doesnt exist
						bzero(buffer, sizeof(buffer));
						strcpy(buffer, UNSUCCESSFUL_REQUEST);
						bytes_read = send(client_fd , buffer , strlen(buffer) , 0);
						// Failure means repeat the process
						continue;
					}
				} else {
					bzero(buffer, sizeof(buffer));
					strcpy(buffer, INVALID_REQUEST);
					bytes_read = send(client_fd , buffer , strlen(buffer) , 0);
					// If command is not 'user' means repeat the process
					continue;
				}

				// Wait for 'pass <password>'
				bzero(buffer, sizeof(buffer));
				bytes_read = recv( client_fd , buffer, MAX_CMD_LEN, 0);
				if (bytes_read == -1) {
					printf("Error during Receiving from Client. Please Reconnect!\n");
					break;
				} else if (bytes_read == 0) {
					//connection broken
					break;
				}
				buffer[bytes_read] = '\0';
				memcpy(command, &buffer[0], command_len);
				command[command_len] = '\0';

				if (strcmp(command, "pass") == 0) {

					int password_len = strlen(buffer) - command_len - 1;
					memcpy(password, &buffer[command_len + 1], password_len);
					password[password_len] = '\0';

					// Check if Password actually exists
					if (check_password(username, password)) {
						// exists
						bzero(buffer, sizeof(buffer));
						strcpy(buffer, SUCCESSFUL_REQUEST);
						bytes_read = send(client_fd , buffer , strlen(buffer) , 0);
						// In case of success , break out of this authentication loop
						break;
					} else {
						// doesnt match
						bzero(buffer, sizeof(buffer));
						strcpy(buffer, UNSUCCESSFUL_REQUEST);
						bytes_read = send(client_fd , buffer , strlen(buffer) , 0);
						// Failure means repeat the process
						continue;
					}
				} else {
					bzero(buffer, sizeof(buffer));
					strcpy(buffer, INVALID_REQUEST);
					bytes_read = send(client_fd , buffer , strlen(buffer) , 0);
					// If command is not 'pass' means repeat the process
					continue;
				}

			}
			/* --- AUTHENTICATION COMPLETE  */


			/*
			* +--------------------+
			* |  File Operations   |
			* +--------------------+
			*/
			while (1) {
				printf("File service waiting...\n");
				fflush(stdout);

				bzero(buffer, sizeof(buffer));
				bytes_read = recv( client_fd , buffer, MAX_CMD_LEN, 0);
				if (bytes_read == -1) {
					printf("Error during Receiving from Client. Please Reconnect!\n");
					break;
				} else if (bytes_read == 0) {
					//connection broken
					break;
				}
				buffer[bytes_read] = '\0';

				int command_len = get_first_word_length(buffer);
				memcpy(command, &buffer[0], command_len);
				command[command_len] = '\0';

				printf("Command Read=> %s\n",buffer);
				fflush(stdout);

				if (strcmp(command, "dir") == 0) {
					// Pointer for directory entry
					struct dirent *dir_entry;

					// opendir() returns a pointer of DIR type.
					
					// printf("dir:: %s\n", current_directory);
					DIR *dr = opendir(current_directory);
					
					// opendir returns NULL if couldn't open directory
					if (dr == NULL) {
						printf("ERROR:: Directory not found\n");
						continue;
					}

					char NULL_STRING[1] ="\0";
					while ((dir_entry = readdir(dr)) != NULL) {
						bzero(buffer, sizeof(buffer));
						strcat(buffer, dir_entry->d_name);
						// printf("file:: %s\n", dir_entry->d_name);
						bytes_read = send(client_fd , buffer , strlen(buffer) , 0);
						bzero(buffer, sizeof(buffer));
						bytes_read = send(client_fd, NULL_STRING, 1, 0);
					}
					bzero(buffer, sizeof(buffer));
					bytes_read = send(client_fd, NULL_STRING, 1, 0);

					closedir(dr);

				} else if (strcmp(command, "cd") == 0) {

					char dirname[MAX_BUF_SIZE];
					int dirlen = strlen(buffer) - command_len - 1;
					memcpy(dirname, &buffer[command_len + 1], dirlen);
					dirname[dirlen] = '\0';

					if (chdir(dirname) == 0) {
						//success

						getcwd(current_directory, sizeof(current_directory));

						bzero(buffer, sizeof(buffer));
						strcpy(buffer, SUCCESSFUL_REQUEST);
						bytes_read = send(client_fd , buffer , strlen(buffer) , 0);

					} else {
						// error
						bzero(buffer, sizeof(buffer));
						strcpy(buffer, UNSUCCESSFUL_REQUEST);
						bytes_read = send(client_fd , buffer , strlen(buffer) , 0);
					}

				} else if (strcmp(command, "get") == 0) {

					/*
					 * ++==============================++
					 * ||          GET                 ||   
					 * ++==============================++
					 */
					
					//  first, extract the filename that the client is requesting
					int start = command_len + 1, end = -1;
					for (int i = start; i < strlen(buffer); i++) {
						if (buffer[i] == ' ') {
							break;
						}
						end = i;
					}

					int filename_len = end - start + 1;
					char filename[filename_len + 10];
					memcpy(filename, &buffer[start], filename_len);
					filename[filename_len] = '\0';

					// printf("%s\n", filename);

					int file_descriptor = open(filename, O_RDONLY);
					if (file_descriptor == -1) {
						// file could not be opened for reading
						bzero(buffer, sizeof(buffer));
						strcpy(buffer, UNSUCCESSFUL_REQUEST);
						bytes_read = send(client_fd , buffer , strlen(buffer) , 0);

						// Dont go forward and wait for a new command 
						continue;
					} else {
						// File successfully opened for reading. Now we have to send file in chunks.
						bzero(buffer, sizeof(buffer));
						strcpy(buffer, SUCCESSFUL_REQUEST);
						bytes_read = send(client_fd , buffer , strlen(buffer) , 0);
					}

					int MAX_CHUNK_SIZE = 300;
					char data_block[MAX_BUF_SIZE], temporary_buffer[MAX_BUF_SIZE];

					bzero(buffer,sizeof(buffer));
					// Read the first chunk and subsequent read() call will decide whether it is 'M' or 'L'
					int bytes_read = read(file_descriptor, buffer, MAX_CHUNK_SIZE);
					buffer[bytes_read] = '\0';

					printf("Started File Transfer...\n");
					fflush(stdout);

					while (1) {
						
						bzero(temporary_buffer, sizeof(temporary_buffer));
						bzero(data_block,sizeof(data_block));

						int next_bytes_read = read(file_descriptor, temporary_buffer, MAX_CHUNK_SIZE);
						temporary_buffer[next_bytes_read] = '\0';

						if(next_bytes_read == 0){
							// no more bytes to read after the data in 'buffer'
							// Reached EOF, add 'L' to the data
							strcpy(data_block, "L");
						}else{
							strcpy(data_block, "M");
						}

						// for(int d=10000;d>=1;d/=10){
						// 	int x = (bytes_read / d)%10;
						// 	char val[2] = {('0'+x),'\0'};
						// 	strcat(data_block, val);
						// }

						// Write 2 bytes denoting chunk length
						int chunk_len = bytes_read;
						memcpy( &data_block[1], &chunk_len, 2);
						// data_block[3] = '\0';
						memcpy(&data_block[3],buffer,bytes_read);
						// strcat(data_block, buffer);
						data_block[HEADER_LEN+bytes_read] = '\0';

						int status = send(client_fd , data_block, HEADER_LEN+bytes_read , 0);
						/* 
						 * This  being  a  TCP  connection,  there  is  no  guarantee  that  the  entire  block  will  reach together to the other side, you should take care of that.  
						 * You should also take care of the fact that the architectures on both sides may be different 
						 * The files can contain anything, do not assume txt files
						 */
						
						// printf("Sending %ld bytes! and status=%d\n", strlen(data_block),status);
						// printf("Sent :%s\n\n\n", data_block);

						// printf("Sent %d bytes : %s\n\n",bytes_read, data_block);
						// fflush(stdout);

						if(next_bytes_read == 0){
							// no more bytes to read after the data in 'buffer'
							break;
						}

						bzero(buffer, MAX_BUF_SIZE);
						memcpy(buffer, temporary_buffer, next_bytes_read); 
						bytes_read = next_bytes_read;
					}

					printf("Done...\n");
					fflush(stdout);
					close(file_descriptor);

				} else if (strcmp(command, "put") == 0) {
					/*
					 * ++==============================++
					 * ||          PUT                 ||   
					 * ++==============================++
					 */
					
					//  first, extract the filename that the client is trying to save into server
					int start = command_len + 1, end = -1;
					for (int i = start; i < strlen(buffer); i++) {
						if (buffer[i] == ' ') {
							start = i+1;
							break;
						}
					}
					end = strlen(buffer)-1;

					int filename_len = end - start + 1;
					char filename[filename_len + 10];
					memcpy(filename, &buffer[start], filename_len);
					filename[filename_len] = '\0';

					int file_descriptor = open(filename,  O_WRONLY | O_CREAT | O_TRUNC , 0644); 
					if (file_descriptor == -1) {
						// file could not be opened for writing
						perror("Put Failed");
						strcpy(buffer, UNSUCCESSFUL_REQUEST);
						bytes_read = send(client_fd , buffer , strlen(buffer) , 0);

						// Dont go forward and wait for a new command 
						continue;
					} else {
						// File successfully opened for reading. Now we have to send file in chunks.
						strcpy(buffer, SUCCESSFUL_REQUEST);
						bytes_read = send(client_fd , buffer , strlen(buffer) , 0);
					}

					// printf("Filename= %s\n",filename);

					int status;

					while(1){
						
						// READ HEADER FIRST
						char header1;
						unsigned short header2;

						bzero(buffer,sizeof(buffer));
						bytes_read=recv(client_fd, buffer, 1, MSG_WAITALL);
						header1 = buffer[0];
						char tempsz[2];
						bzero(buffer,sizeof(buffer));
						bytes_read = recv(client_fd, tempsz, 2, MSG_WAITALL);
						memcpy(&header2, &tempsz, sizeof(tempsz));
					
						// printf("Bytes_READ=%d, header1=%c, header2 = %d\n",bytes_read, header1, header2);
						// fflush(stdout);
						
						int read_left = header2;

						while(read_left > 0){
							// printf("=%d left\n",read_left);
							bzero(buffer,MAX_BUF_SIZE);
							bytes_read=recv(client_fd,buffer, min(read_left,MAX_BUF_SIZE), 0);
							// buffer[bytes_read] = '\0';
							status = write(file_descriptor, buffer, bytes_read);
							if(status < 0){
								printf("put_error():: write() failed!\n");
								// close(file_descriptor);
								break;
							}
							if(bytes_read < 0){
								perror("recv() failed");
								continue;
							}
							read_left -= bytes_read;
						}

						// printf("Left=> %d\n",read_left);

						if(header1 == 'L'){
							// printf("L found\n");
							break;
						}
					}

					close(file_descriptor);

				} else {
					// strcpy(buffer, "Your Command is Invalid!");
					// // bytes_read = send(client_fd , buffer , strlen(buffer) , 0);
					
					// DO NOTHING
				}

				printf("Reinitiating File Services!\n---------------\n");
			}


			/* --- HANDLE CLOSING THE CONNECTION */
			printf("Connection Closed by the Client!\n");
			// child terminates
			close(client_fd);
			// printf("child exiting\n");
			exit(0);
		}else{
			close(client_fd);
		}
	}

	return 0;
}
