/*
 * Name - Debanjan Saha
 * Roll - 19CS30014
 * Assignment - 1
 * Description - UDP Server
 * Networks_Lab
*/
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>

#define PORT 8181
#define BACKLOG 10
#define MAX_BUF_SIZE 100

int is_alphanumeric(char ch){
    if( (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ){
        return 1;
    }
    return 0;
}

int is_fullstop(char ch){
    if(ch == '.'){
        return 1;
    }
    return 0;
}

int get_statistics(char* str, char* msg, int running_chars, int* words, int* sentences, int* characters){
    int len = strlen(str);

    *characters += len;

    for(int i = 0; i < len; i++){
        char ch = str[i];
        if(is_fullstop(ch)){
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
	int sockfd;
	struct sockaddr_in server_address, client_address;
    socklen_t addrlen = sizeof(client_address);
    int bytes_read;

    char buffer[MAX_BUF_SIZE] = {0};
	
	/*
     * Create the server side socket file descriptor
    */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1){
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
    |<<< UDP Server is up and running  >>>|\n\
    |<<<                               >>>|\n\
    |<<<     Press Ctrl + C to stop    >>>|\n\
    +-------------------------------------+\n\n");

    while(1){
        
        int running_chars = 0;
        int words = 0, sentences = 0, characters = 0;
        int chunk_count = 0;
        char msg_from_server[MAX_BUF_SIZE];

        printf("Waiting for a client...\n\n");

        // Keep Reading from the client
        
        for(;;){
            bytes_read = recvfrom( sockfd , buffer, MAX_BUF_SIZE, 0, (struct sockaddr *) &client_address, &addrlen);
            buffer[bytes_read] = '\0';
            // printf("bytes_read = %d\n",bytes_read);

            if(bytes_read > 0){
                chunk_count += 1;
                printf("[STATUS] Received Chunk-%d\n",chunk_count);
                // printf("%s\n",buffer );
                running_chars = get_statistics(buffer, msg_from_server, running_chars, &words, &sentences, &characters); 
                // printf("\nSending Statistics to client for this chunk.\n");
            } else {
                /*
                 * When I receive an empty string, it means, file transfer is completed. 
                 * Hence, send the appropriate message back to client and reset for new connections.
                */
                sendto(sockfd , msg_from_server , strlen(msg_from_server) , 0,  (struct sockaddr *) &client_address, addrlen);
                printf("\nFile Received Completely.\nStatistics has been sent back!\n\n");
                break;
            }
        }

        printf("------------------------------\n\n");
    }

	return 0;
}