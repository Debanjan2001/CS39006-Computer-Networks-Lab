#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define CMDPROMPT "myFTP>"
#define CMDSUCCESS "Command executed successfully.\n"

#define ECMDLEN 1000
#define CMDLEN 50
#define ARGSLEN 500
#define IPLEN 50
#define PORTLEN 10
#define UNAMELEN 250
#define PASSLEN 250
#define DIRPATHLEN 450
#define MSGBLOCKLEN 1000

#define OPEN "open"
#define USER "user"
#define PASS "pass"
#define CHDIR "cd"
#define LOCALCD "lcd"
#define DIR "dir"
#define GET "get"
#define PUT "put"
#define MGET "mget"
#define MPUT "mput"
#define QUIT "quit"

#define ERRLOGIN 600
#define ERRINV 500
#define OK 200

#define PORT 65534
#define ERROR_EXIT -1

void trim(char* str) {
    int n = strlen(str);
    for (int i = n-1; i >= 0; i--) {
        if(isspace(str[i])) { str[i] = '\0'; }
        else break;
    }
    // trim from the front
}

int main (int argc, char const * argv[]) {

    int _DEBUG = (argc < 2) ? 0 : atoi(argv[1]);
    if(_DEBUG) {
        printf(">> Debug Mode : ON\n\n");
    }


    printf("Starting the Server... Binding...\n");

    // Creating a socket descriptor for the server
    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    if (serv_sock == -1) {
        printf("ERROR:: cannot create socket. \n");
        exit(ERROR_EXIT);
    }

    // Socket descriptor created successfully
    if (_DEBUG) {
        printf(">>D Socket Descriptor : %d\n", serv_sock);
    }

    // ...
    if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        printf("ERROR:: setsockopt() error.\n");
        exit(ERROR_EXIT);
    }

    // Binding the socket to service function
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(serv_sock, (struct  sockaddr *)&address, sizeof(address)) == -1) {
        printf("ERROR:: bind() failed.\n");
        exit(ERROR_EXIT);
    }

    // Bind successful
    printf("\t>> Bind Successful.\n");

    // Begin listening for connection requests
    if (listen(serv_sock, 5) == -1) {
        printf("ERROR:: listen() failure.\n");
        exit(ERROR_EXIT);
    }

    // Listening successfully
    printf("\t>> Server listening for connections requests...\n");
    int sock = -1, recieved_status = -1;
    addrlen = sizeof(address);
    sock = accept(serv_sock, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    if (sock == -1) {
        printf("ERROR:: failed to accept client connection request.\n");
        exit(ERROR_EXIT);
    }
    

    char entered_cmd[ECMDLEN];
    char cmd[CMDLEN];
    char args[ARGSLEN];
    int c = 0;

    int login = 2;
    int connected = 1;

    char buffer[MSGBLOCKLEN], rcbuffer[MSGBLOCKLEN];
    memset(buffer, 0, sizeof(buffer));
    memset(rcbuffer, 0, sizeof(rcbuffer));

    char rootdir[DIRPATHLEN] = "./";

    while(1) {
        int char_read = recv(sock, rcbuffer, MSGBLOCKLEN, 0);
        if (char_read < 0) {
            printf("ERROR:: failed to recieve a response from the server.\n");
            continue;
        }
        

        
        if(_DEBUG) { printf(">> The command entered is [%ld]: [%s] \n", strlen(rcbuffer), rcbuffer); }
        
        sscanf(rcbuffer, "%s%[^\n]", cmd, args);

        if(strcmp(cmd, USER) == 0) {
        // 2. user
            char username[UNAMELEN];
            sscanf(args, "%s", username);
            if(_DEBUG) { printf(">> USER : Username = [%s]\n", username); }

            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "%s %s", cmd, username);
            if(_DEBUG) { printf(">> message to be sent [%ld bytes] = [%s]\n", strlen(buffer), buffer); }

            if(send(sock, buffer, MSGBLOCKLEN, 0) < 0) {
                printf("ERROR:: message sending to the server failed.\n");
                continue;
            }

            memset(rcbuffer, 0, sizeof(rcbuffer));
            int char_read = recv(sock, rcbuffer, MSGBLOCKLEN, 0);
            if (char_read < 0) {
                printf("ERROR:: failed to recieve a response from the server.\n");
                continue;
            }

            int rc_code = atoi(rcbuffer);
            if(rc_code == ERRLOGIN) {
                printf("ERROR[code:%d] :: authenticate client first.\n", ERRLOGIN);
                login = 0;
                continue;
            }
            else if(rc_code == ERRINV) {
                printf("ERROR[code:%d] :: user does not exist.\n", ERRINV);
                login = 0;
                continue;
            }
            else if(rc_code == OK) {
                printf("User [%s] validated. \n", username);
                printf(CMDSUCCESS);
                login += 1;
            }
            else {
                printf("ERROR:: unknown response recieved from server.\n");
                continue;
            }
        }
        else if( strcmp(cmd, PASS) == 0) {
        // 3. pass
            char password[PASSLEN];
            sscanf(args, "%s", password);
            if(_DEBUG) { printf(">> PASS : Password = [%s]\n", password); }

            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "200");
            if(_DEBUG) { printf(">> message to be sent [%ld bytes] = [%s]\n", strlen(buffer), buffer); }

            if(send(sock, buffer, MSGBLOCKLEN, 0) < 0) {
                printf("ERROR:: message sending to the server failed.\n");
                login = 0;
                continue;
            }
        }
        else if(strcmp(cmd, CHDIR) == 0) {
        // 4. cd
            char dir[DIRPATHLEN];
            sscanf(args, "%s", dir);
            if(_DEBUG) { printf(">> CHDIR : Directory = [%s]\n", dir); }

            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "%s %s", cmd, dir);
            if(_DEBUG) { printf(">> message to be sent [%ld bytes] = [%s]\n", strlen(buffer), buffer); }

            if(send(sock, buffer, MSGBLOCKLEN, 0) < 0) {
                printf("ERROR:: message sending to the server failed.\n");
                continue;
            }
        }
        else if(strcmp(cmd, LOCALCD) == 0) {
        // 5. lcd
            char dir[DIRPATHLEN];
            sscanf(args, "%s", dir);
            if(_DEBUG) { printf(">> LOCALCD : Directory = [%s]\n", dir); }

            char newdir[DIRPATHLEN];
            if (chdir(dir) < 0) {
                printf("ERROR:: cannot change directory.\n");
                continue;
            }
            else {
                printf("Current working directory changed to [%s]\n", getcwd(newdir, DIRPATHLEN));
                printf(CMDSUCCESS);
            }
        }
        else if(strcmp(cmd, DIR) == 0) {
        // 6. dir
            if(_DEBUG) { printf(">> DIR\n"); }

            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "%s", cmd);
            if(_DEBUG) { printf(">> message to be sent [%ld bytes] = [%s]\n", strlen(buffer), buffer); }

            if(send(sock, buffer, MSGBLOCKLEN, 0) < 0) {
                printf("ERROR:: message sending to the server failed.\n");
                continue;
            }
            printf("Response recieved from server : \n");
            int char_read = 0;
            while(1) {
                memset(rcbuffer, 0, sizeof(rcbuffer));
                char_read = recv(sock, rcbuffer, MSGBLOCKLEN, 0);
                if (char_read < 0) {
                    printf("ERROR:: failed to recieve a response from the server.\n");
                    break;
                }

                if(rcbuffer[0] == 0) {
                    break;
                } 
                else {
                    printf("\t>> %s \n", rcbuffer);
                }
            }

            if(char_read >= 0) {
                printf(CMDSUCCESS);
            }
            else {
                printf("Terminating recieving due to error.\n");
            }
        }
        else if(strcmp(cmd, GET) == 0) {
        // 7. get
            if(_DEBUG) { printf(">> PUT\n"); }

            char remoteFile[DIRPATHLEN], localFile[DIRPATHLEN];
            sscanf(args, "%s%s", remoteFile, localFile);

            memset(buffer, 0, MSGBLOCKLEN);
            int targetFile = -1;
            if ((targetFile = open(remoteFile, O_RDONLY)) < 0) { 
                strcpy(buffer, "500");
            }
            else {
                strcpy(buffer, "200");
            }

            if(_DEBUG) {printf(">> LOCALFILE: access available.\n");}

            if (send(sock, buffer, MSGBLOCKLEN, 0) < 0) {
                printf("ERROR:: message sending to the server failed.\n");
                continue;
            }

            int char_read = -1;
            int terminate = 0;

            int tempsize = MSGBLOCKLEN-4;
            char temp[tempsize];
            memset(temp, 0, sizeof(temp));
            char_read = -1;
            while(1) {
                if((char_read = read(targetFile, temp, tempsize)) < 0) {
                    printf("ERROR:: read() for file [%s] failed.\n", remoteFile);
                    break;
                }
                char block_type = 'M';

                if(char_read < tempsize) {
                    block_type = 'L';
                    char_read = strlen(temp);
                }

                short int charread = htons(char_read);
                char bytesize[2];
                memcpy(bytesize, &charread, sizeof(bytesize));

                if(_DEBUG) {printf(">> file content to be transmitted = [%d bytes]\n",(int)char_read);}
                memset(buffer, 0, MSGBLOCKLEN);
                sprintf(buffer, "%c%s%s", block_type, bytesize, temp);
                if(_DEBUG) { printf(">> message to be sent [%ld bytes] = [%s]\n", strlen(buffer), buffer); }

                if(send(sock, buffer, MSGBLOCKLEN, 0) < 0) {
                    printf("ERROR:: message sending to the server failed. remote file content may not be trusted.\n");
                    continue;
                }

                if(block_type == 'L') {
                    break;
                }
            }

            if (char_read >= 0) {
                printf(CMDSUCCESS);
            }
            else {
                printf("Terminating recieving due to error.\n");
            }
            
        }
        else if(strcmp(cmd, PUT) == 0) {
        // 8. put
            if(_DEBUG) { printf(">> GET\n"); }

            char remoteFile[DIRPATHLEN], localFile[DIRPATHLEN];
            sscanf(args, "%s%s", localFile, remoteFile);

            memset(buffer, 0, MSGBLOCKLEN);
            int targetFile = -1;
            if ((targetFile = open(remoteFile, O_WRONLY | O_CREAT)) < 0) { 
                strcpy(buffer, "500");
            }
            else {
                strcpy(buffer, "200");
            }
            if(_DEBUG) {printf(">> LOCALFILE: access available.\n");}

            if (send(sock, buffer, MSGBLOCKLEN, 0) < 0) {
                printf("ERROR:: message sending to the server failed.\n");
                continue;
            }
            int char_read = -1;
            int terminate = 0;

            if(_DEBUG) {printf(">> TRANSFER: recieving file now.\n");}

            while(1) {
                memset(rcbuffer, 0, MSGBLOCKLEN);
                if ((char_read = recv(sock, rcbuffer, MSGBLOCKLEN, 0)) < 0) {
                    printf("ERROR:: failed to recieve a server response.\n");
                    break;
                }
                if(_DEBUG) {printf(">> message recieved [%ld bytes] : [%s]\n", strlen(rcbuffer), rcbuffer);}
                terminate = (rcbuffer[0] == 'L');

                short int bsize;
                char bsizebuffer[2];
				bsizebuffer[0] = rcbuffer[1];
				bsizebuffer[1] = rcbuffer[2];
				memcpy(&bsize, &bsizebuffer, sizeof(bsizebuffer));
				int blocksize = ntohs(bsize);

                if(_DEBUG) {printf(">> file content [%d bytes]\n",blocksize); }
				int tempsize = MSGBLOCKLEN-4;
            	char temp[tempsize];
            	memset(temp, 0, tempsize);
            	strcpy(temp, rcbuffer + 3);
                //rcbuffer[blocksize] = 0;
                if(write(targetFile, temp, blocksize) < 0) {
                    printf("ERROR:: write failed on file [%s]. contents may not be trusted.\n", remoteFile);
                    continue;
                }

                if(terminate) {
                    break;
                }
            }

            if (char_read >= 0) {
                printf(CMDSUCCESS);
            }
            else {
                printf("Terminating recieving due to error.\n");
            }
            
        }
        else if(strcmp(cmd, MGET) == 0) {
        // 9. mget
            if(_DEBUG) { printf(">> MGET\n"); }
        }
        else if(strcmp(cmd, MPUT) == 0) {
        // 10. mput
            if(_DEBUG) { printf(">> MPUT\n"); }
        }
        else if(strcmp(cmd, QUIT) == 0) {
        // 11. quit
            if(_DEBUG) { printf(">> QUIT\n"); }
            printf("Exiting...\n");
            break;
        }
        else {
            if(_DEBUG) { printf(">> INVALID\n"); }
            printf("Invalid command.\n");
            continue;
        }
    }



}