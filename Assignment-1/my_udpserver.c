// Server side C/C++ program to demonstrate Socket programming
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
	int sockfd;
	struct sockaddr_in server_address, client_address;
    socklen_t addrlen = sizeof(client_address);
    int val_read;

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

    printf("\
    ***********************\n\
    *     UDP SERVER UP   *\n\
    ***********************\n\n");
    

    while(1){
        
        int running_chars = 0;
        int words = 0, sentences = 0, characters = 0;

        char msg_from_server[MAX_BUF_SIZE];

        // Keep Reading from the client
        
        for(;;){
            val_read = recvfrom( sockfd , buffer, MAX_BUF_SIZE, MSG_WAITALL, (struct sockaddr *) &client_address, &addrlen);
            buffer[val_read] = '\0';
            printf("val_read = %d\n",val_read);

            if(val_read > 0){
                printf("Receiving...\n");
                // printf("%s\n",buffer );
                running_chars = get_statistics(buffer, msg_from_server, running_chars, &words, &sentences, &characters); 
                // printf("\nSending Statistics to client for this chunk.\n");
                sendto(sockfd , msg_from_server , strlen(msg_from_server) , 0,  (struct sockaddr *) &client_address, addrlen);
                printf("\n");
            } else {
                sendto(sockfd , msg_from_server , strlen(msg_from_server) , 0,  (struct sockaddr *) &client_address, addrlen);
                break;
            }
        }
    }

	return 0;
}