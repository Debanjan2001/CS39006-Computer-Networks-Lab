// Server side C/C++ program to demonstrate Socket programming
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>

#define PORT 8080
#define BACKLOG 10
#define BUFFER_SIZE 70
#define MAX_BUF_SIZE 100

int is_alphanumeric(char ch){
    if( (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ){
        return 1;
    }
    return 0;
}

int get_statistics(char* str, char* msg, int running_chars, int* words, int* sentences, int* characters){
    int len = strlen(str);

    *characters += len;

    for(int i = 0; i < len; i++){
        char ch = str[i];
        if(ch == '.'){
            *sentences += 1;
            if(running_chars>0){
                *words += 1;
            }
            running_chars = 0;
        }
        if(isspace(ch)){
            if(running_chars>0){
                *words += 1;
            }
            running_chars = 0;
        }

        if( is_alphanumeric(ch) ){
            running_chars += 1;
        }
    }
    sprintf(msg,"#Characters = %d, #Words = %d, #Sentences = %d.", *characters, *words, *sentences);
    return running_chars;
}

int main(int argc, char const *argv[])
{
	int server_fd, client_fd;
	struct sockaddr_in server_address, client_address;
    int addrlen = sizeof(client_address);
    int val_read;

    char buffer[MAX_BUF_SIZE] = {0};
	
	/*
     * Create the server side socket file descriptor
    */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1){
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}
	
    /*
	 * Forcefully attaching socket to the port 8080
	*/
    int force = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &force, sizeof(force)) == -1){
		perror("Unable to forcefully attach port");
		exit(EXIT_FAILURE);
	}

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons( PORT );
	
    /*
	 * Bind the socket to the port 8080
	*/
    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}
	
    if (listen(server_fd, BACKLOG) < 0)
    {
        perror("Error while listening for incoming connections");
        exit(EXIT_FAILURE);
    }

    printf("\
    ***********************\n\
    *     TCP SERVER UP   *\n\
    ***********************\n\n");
    

    while(1){
        
        client_fd = accept(server_fd, (struct sockaddr *)& client_address, (socklen_t*)& addrlen );
        
        if (client_fd < 0){
            perror("Failed to Accept Connection");
            exit(EXIT_FAILURE);
        }

        int running_chars = 0;
        int words = 0, sentences = 0, characters = 0;
        // Keep Reading from the client
        for(;;){
            val_read = recv( client_fd , buffer, MAX_BUF_SIZE, 0);
            buffer[val_read] = '\0';
           
            if(val_read > 0){
                printf("Receiving...\n");
                // printf("%s\n",buffer );
                // printf("val_read = %d\n",val_read);
                char msg_from_server[BUFFER_SIZE];
                running_chars = get_statistics(buffer, msg_from_server, running_chars, &words, &sentences, &characters); 
                // printf("\nSending Statistics to client for this chunk.\n");
                send(client_fd , msg_from_server , strlen(msg_from_server) , 0);
                printf("\n");
            } else {
                int status = shutdown(client_fd, SHUT_RDWR);
                if (status < 0)
                {
                    perror("Error during connection shutdown");
                    exit(EXIT_FAILURE);
                }
                close(client_fd);
                break;
            }
        }
       
    }

	return 0;
}