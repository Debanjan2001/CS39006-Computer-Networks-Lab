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

const char* CMDPROMPT ="myFTP>";
const char* CMDSUCCESS= "Command executed successfully.\n";

#define ECMDLEN 1000
#define CMDLEN 50
#define ARGSLEN 500
#define IPLEN 50
#define PORTLEN 10
#define UNAMELEN 250
#define PASSLEN 250
#define DIRPATHLEN 450
#define MSGBLOCKLEN 1000
#define ERRCODELEN 3
#define HEADERLEN 3

const char* OPEN ="open";
const char* USER ="user";
const char* PASS ="pass";
const char* CHDIR = "cd";
const char* LOCALCD = "lcd";
const char* DIR= "dir";
const char* GET= "get";
const char* PUT= "put";
const char* MGET ="mget";
const char* MPUT ="mput";
const char* QUIT ="quit";

const int ERRLOGIN = 600;
const int ERRINV = 500;
const int OK = 200;

void trim(char* str) {
	int n = strlen(str);
	for (int i = n-1; i >= 0; i--) {
		if(isspace(str[i])) { str[i] = '\0'; }
		else break;
	}
	// trim from the front
}

int min(int x1, int x2) {
	return (x1<x2) ? x1 : x2;
}

int handleGet(int sock, const char* cmd, char* remoteFile, char* localFile, int _DEBUG) {
	char buffer[MSGBLOCKLEN], rcbuffer[MSGBLOCKLEN];
	
	int targetFile = -1;
	if ((targetFile = open(localFile, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
		printf("ERROR:: cannot be opened target file on the local client; no request sent to the server...\n");
		return -1;
	}

	if(_DEBUG) {printf(">> LOCALFILE: access available.\n");}

	memset(buffer, 0, MSGBLOCKLEN);
	sprintf(buffer, "%s %s", cmd, remoteFile);

	if (send(sock, buffer, strlen(buffer), 0) < 0) {
		printf("ERROR:: message sending to the server failed.\n");
		return -1;
	}
	int char_read = -1;
	int terminate = 0;

	memset(rcbuffer, 0, MSGBLOCKLEN);
	if ((char_read = recv(sock, rcbuffer, ERRCODELEN, MSG_WAITALL)) < 0) {
		printf("ERROR:: failed to recieve a server response.\n");
		return -1;
	}
	printf("%d : %s", char_read, rcbuffer);
	int rc_code = atoi(rcbuffer);
	printf("%d", rc_code);
	if(rc_code == ERRINV) {
		printf("ERROR[code:%d] :: remote file does not exist.\n", ERRINV);
		return -1;
	}
	else if(rc_code == OK) {
		printf("Remote file found. Transferring now... \n");
	}
	else {
		printf("ERROR:: unknown response recieved from server.\n");
		return -1;
	}

	if(_DEBUG) {printf(">> TRANSFER: recieving file now.\n");}
	char writeBuffer[2*MSGBLOCKLEN];
	memset(writeBuffer, 0, sizeof(writeBuffer));

	int leftover = 0;
	int writeSize = 0;
	int lastFlag = 0;
	int chunkSize = 0;

	char headerBuffer[3];
	int globalOffset = 0;

	while(1) {
		memset(rcbuffer, 0, MSGBLOCKLEN);
		if ((char_read = recv(sock, rcbuffer, HEADERLEN, MSG_WAITALL)) < 0) {
			printf("ERROR:: failed to recieve a server response.\n");
			break;
		}
		lastFlag = (rcbuffer[0] == 'L');
		unsigned short temp, msgsize;	
		char tempsz[2];
		tempsz[0] = rcbuffer[1];
		tempsz[1] = rcbuffer[2];
		memcpy(&temp, &tempsz, sizeof(tempsz));
		// msgsize = ntohs(temp);
		chunkSize = leftover = temp;
		printf("leftover = %d\n",leftover);

		while(leftover > 0) {
			if(_DEBUG) {printf(">> message recieved [%ld bytes] : [%s]\n", strlen(rcbuffer), rcbuffer);}
			memset(rcbuffer, 0, sizeof(rcbuffer));
			if ((char_read = recv(sock, rcbuffer, min(leftover, sizeof(rcbuffer)), 0)) < 0) {
				printf("ERROR:: failed to recieve a server response.\n");
				break;
			}
			printf("charread = %d\n",char_read);
			if(char_read < min(leftover, MSGBLOCKLEN)) {
				printf("recv() maybe problematic.\n");
			}
			if(write(targetFile, rcbuffer, char_read) < 0) {
				printf("ERROR:: write failed on file [%s]. contents may not be trusted.\n", remoteFile);
				exit(EXIT_FAILURE);
			}
			leftover -= char_read;
		}

		if(lastFlag) break;
	}
	if (char_read >= 0) {
		printf("%s",CMDSUCCESS);
	}
	else {
		printf("Terminating recieving due to error.\n");
		return -1;
	}
	close(targetFile);
	return 0;
}

int handlePut(int sock, const char* cmd, char* localFile, char* remoteFile, int _DEBUG) {
	char buffer[MSGBLOCKLEN], rcbuffer[MSGBLOCKLEN];
	int targetFile = -1;
	if ((targetFile = open(localFile, O_RDONLY)) < 0) {
		printf("ERROR:: cannot be opened target file on the local client; no request sent to the server...\n");
		return -1;
	}

	if(_DEBUG) {printf(">> LOCALFILE: access available.\n");}

	memset(buffer, 0, MSGBLOCKLEN);
	sprintf(buffer, "%s %s %s", cmd, localFile, remoteFile);

	if (send(sock, buffer, strlen(buffer), 0) < 0) {
		printf("ERROR:: message sending to the server failed.\n");
		return -1;
	}

	int char_read = -1;
	int terminate = 0;

	memset(rcbuffer, 0, MSGBLOCKLEN);
	if ((char_read = recv(sock, rcbuffer, ERRCODELEN, 0)) < 0) {
		printf("ERROR:: failed to recieve a server response.\n");
		return -1;
	}

	int rc_code = atoi(rcbuffer);
	if(rc_code == ERRINV) {
		printf("ERROR[code:%d] :: remote file access denied.\n", ERRINV);
		return -1;
	}
	else if(rc_code == OK) {
		printf("Remote file found. Transferring now... \n");
	}
	else {
		printf("ERROR:: unknown response recieved from server.\n");
		return -1;
	}
	int tempsize = MSGBLOCKLEN-3;
	char data[tempsize], temp[tempsize];
	memset(data, 0, sizeof(data));
	if((char_read = read(targetFile, data, tempsize)) < 0) {
		printf("ERROR:: read() for file [%s] failed.\n", localFile);
		return -1;
	}
	int next_char_read;
	while(1) {
		memset(temp,0,sizeof(temp));
		memset(buffer,0,sizeof(buffer));

		if((next_char_read = read(targetFile, temp, tempsize)) < 0) {
			printf("ERROR:: read() for file [%s] failed.\n", localFile);
			break;
		}
		
		if(next_char_read == 0) {
			strcpy(buffer, "L");
		}else{
			strcpy(buffer, "M");
		}

		int chunk_len = char_read;
		memcpy( &buffer[1], &chunk_len, 2);
		// buffer[3] = '\0';
		memcpy(&buffer[3],data,char_read);
		// strcat(buffer, buffer);
		buffer[HEADERLEN+char_read] = '\0';
		

		if(_DEBUG) { printf(">> message to be sent [%ld bytes] = [%s]\n", strlen(buffer), buffer); }

		if(send(sock, buffer, char_read+HEADERLEN, 0) < 0) {
			printf("ERROR:: message sending to the server failed. remote file content may not be trusted.\n");
			return -1;
		}

		if(next_char_read == 0) {
			break;
		}

		memset(data,0,sizeof(data));
		memcpy(data, temp, next_char_read);
		char_read = next_char_read;
	}

	if (char_read >= 0) {
		printf("%s",CMDSUCCESS);
	}
	else {
		printf("Terminating recieving due to error.\n");
		return -1;
	}

	close(targetFile);
	return 0;
}



int main (int argc, char const * argv[]) {

	int _DEBUG = (argc < 2) ? 0 : atoi(argv[1]);
	if(_DEBUG) {
		printf(">> Debug Mode : ON\n\n");
	}

	char entered_cmd[ECMDLEN];
	char cmd[CMDLEN];
	char args[ARGSLEN];
	int c = 0;

	int login = 0;
	int connected = 0;

	int sock = -1;
	struct sockaddr_in serv_addr;

	char buffer[MSGBLOCKLEN], rcbuffer[MSGBLOCKLEN];
	memset(buffer, 0, sizeof(buffer));
	memset(rcbuffer, 0, sizeof(rcbuffer));

	char rootdir[DIRPATHLEN] = "./";

	while(1) {



		printf("%s ", CMDPROMPT);
		fgets(entered_cmd, CMDLEN, stdin);
		trim(entered_cmd);

		if(_DEBUG) { printf(">> The command entered is [%ld]: [%s] \n", strlen(entered_cmd), entered_cmd); }
		
		sscanf(entered_cmd, "%s%[^\n]", cmd, args);



		if(strcmp(cmd, OPEN) == 0) {
		// 1. open
			char ip[IPLEN], port[PORTLEN];
			sscanf(args, "%s%s", ip, port);
			if(_DEBUG) { printf(">> OPEN : IP_Addr = [%s], Port = [%s]\n", ip, port); }
				
			sock = socket(AF_INET, SOCK_STREAM, 0);
			if (sock == -1) {
				printf("ERROR:: cannot create socket.\n");
				continue;
			}
			if(_DEBUG) { printf(">> SOCKET: created. socket descriptor : [%d]\n", sock); }

			serv_addr.sin_family = AF_INET;
			int port_int = atoi(port);
			if (port_int < 20000 || port_int > 65535) {
				printf("ERROR:: invalid port.\n");
				continue;
			}
			serv_addr.sin_port = htons(port_int);
			if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
				printf("ERROR:: invalid/unsupport ip address.\n");
				continue;
			}

			if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) {
				printf("ERROR:: connection to server failed.\n");
				continue;
			}
			
			connected = 1;
			printf("Connected to the server @ [%s:%s]!!\n", ip, port);
			fflush(stdout);
			printf("%s",CMDSUCCESS);
		}




		else if(connected == 1 && strcmp(cmd, USER) == 0) {
		// 2. user
			char username[UNAMELEN];
			sscanf(args, "%s", username);
			if(_DEBUG) { printf(">> USER : Username = [%s]\n", username); }

			memset(buffer, 0, sizeof(buffer));
			sprintf(buffer, "%s %s", cmd, username);
			if(_DEBUG) { printf(">> message to be sent [%ld bytes] = [%s]\n", strlen(buffer), buffer); }

			if(send(sock, buffer, strlen(buffer), 0) < 0) {
				printf("ERROR:: message sending to the server failed.\n");
				continue;
			}

			memset(rcbuffer, 0, sizeof(rcbuffer));
			int char_read = recv(sock, rcbuffer, ERRCODELEN, 0);
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
				printf("%s",CMDSUCCESS);
				login += 1;
			}
			else {
				printf("ERROR:: unknown response recieved from server.\n");
				continue;
			}
		}




		else if(connected == 1 && strcmp(cmd, PASS) == 0) {
		// 3. pass
			char password[PASSLEN];
			sscanf(args, "%s", password);
			if(_DEBUG) { printf(">> PASS : Password = [%s]\n", password); }

			memset(buffer, 0, sizeof(buffer));
			sprintf(buffer, "%s %s", cmd, password);
			if(_DEBUG) { printf(">> message to be sent [%ld bytes] = [%s]\n", strlen(buffer), buffer); }

			if(send(sock, buffer, strlen(buffer), 0) < 0) {
				printf("ERROR:: message sending to the server failed.\n");
				login = 0;
				continue;
			}

			memset(rcbuffer, 0, sizeof(rcbuffer));
			int char_read = recv(sock, rcbuffer, ERRCODELEN, 0);
			if(_DEBUG) { printf(">> message recieved [%d bytes] = [%s]\n", char_read, rcbuffer); }

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
				printf("ERROR[code:%d] :: invalid password.\n", ERRINV);
				login = 0;
				continue;
			}
			else if(rc_code == OK) {
				printf("Password [%s] validated. \n", password);
				printf("%s",CMDSUCCESS);
				login += 1;
			}
			else {
				printf("ERROR:: unknown response recieved from server.\n");
				continue;
			}
		}




		else if(connected == 1 && login == 2 && strcmp(cmd, CHDIR) == 0) {
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

			memset(rcbuffer, 0, sizeof(rcbuffer));
			int char_read = recv(sock, rcbuffer, ERRCODELEN, 0);
			if (char_read < 0) {
				printf("ERROR:: failed to recieve a response from the server.\n");
				continue;
			}

			int rc_code = atoi(rcbuffer);
			if(rc_code == ERRINV) {
				printf("ERROR[code:%d] :: invalid password.\n", ERRINV);
				continue;
			}
			else if(rc_code == OK) {
				printf("Server directory changed to [%s] \n", dir);
				printf("%s",CMDSUCCESS);
			}
			else {
				printf("ERROR:: unknown response recieved from server.\n");
				continue;
			}
		}




		else if(connected == 1 && login == 2 && strcmp(cmd, LOCALCD) == 0) {
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
				printf("%s",CMDSUCCESS);
			}
		}



		else if(connected == 1 && login == 2 && strcmp(cmd, DIR) == 0) {
		// 6. dir
			if(_DEBUG) { printf(">> DIR\n"); }

			memset(buffer, 0, sizeof(buffer));
			strcpy(buffer, cmd);
			if(_DEBUG) { printf(">> message to be sent [%ld bytes] = [%s]\n", strlen(buffer), buffer); }

			if(send(sock, buffer, strlen(buffer), 0) < 0) {
				printf("ERROR:: message sending to the server failed.\n");
				continue;
			}
			printf("Response recieved from server : \n");
			int char_read = 0;
			int flag = 0, over = 0;
			char filename[DIRPATHLEN];
			int pointer = 0;
			fflush(stdout);
			while(1) {
				memset(rcbuffer, 0, sizeof(rcbuffer));
				char_read = recv(sock, rcbuffer, MSGBLOCKLEN, 0);
				if (char_read < 0) {
					printf("ERROR:: failed to recieve a response from the server.\n");
					break;
				}
				for(int i = 0; i < char_read; i++) {
					if(i < char_read-1 && rcbuffer[i] == '.' && rcbuffer[i+1] == '1') {
						printf("Bad\n");
					}

					if(rcbuffer[i] == 0) {
						flag += 1;
						if(flag == 1) {
							fflush(stdout);
							filename[pointer] = '\n';
							printf(">> %s", filename);
							memset(filename,0,sizeof(filename));
							pointer = 0;
						}
						else if(flag == 2) {
							over = 1;
							break;
						}
					}
					else{ 
						flag = 0;
						filename[pointer] = rcbuffer[i];
						pointer++;
					}
				}
				if(over) break;
			}

			if(char_read >= 0) {
				printf("%s",CMDSUCCESS);
			}
			else {
				printf("Terminating recieving due to error.\n");
			}
		}



		else if(connected == 1 && login == 2 && strcmp(cmd, GET) == 0) {
		// 7. get
			if(_DEBUG) { printf(">> GET\n"); }

			char remoteFile[DIRPATHLEN], localFile[DIRPATHLEN];
			sscanf(args, "%s%s", remoteFile, localFile);

			if(handleGet(sock, cmd, remoteFile, localFile, _DEBUG) < 0) {
				continue;
			}
			
		}



		else if(connected == 1 && login == 2 && strcmp(cmd, PUT) == 0) {
		// 8. put
			if(_DEBUG) { printf(">> PUT\n"); }

			char remoteFile[DIRPATHLEN], localFile[DIRPATHLEN];
			sscanf(args, "%s%s", localFile, remoteFile);

			if(handlePut(sock, cmd, localFile, remoteFile, _DEBUG) < 0) {
				continue;
			}
			
		}

		else if(connected == 1 && login == 2 && strcmp(cmd, MGET) == 0) {
		// 9. mget
			if(_DEBUG) { printf(">> MGET\n"); }

			char filename[DIRPATHLEN];
			memset(filename, 0, sizeof(filename));
			int pointer = 0;
			strcat(args, "\n");
			for(int i = 0; i < sizeof(args); i++) {
				if(args[i] == '\n' || args[i] == ',') {
					char copyfilename[DIRPATHLEN];
					strcpy(copyfilename, filename);
					// printf(">[%s]\n", filename);
					if(handleGet(sock, GET, copyfilename, filename, _DEBUG) < 0) {
						printf("ERROR:: MGET() failed on [%s]\n",filename);
						break;
					}
					memset(filename, 0, sizeof(filename));
					pointer = 0;
				}
				else if(!isspace(args[i])) {
					filename[pointer] = args[i];
					pointer++;
				}
			}
		}



		else if(connected == 1 && login == 2 && strcmp(cmd, MPUT) == 0) {
		// 10. mput
			if(_DEBUG) { printf(">> MPUT\n"); }

			char filename[DIRPATHLEN];
			memset(filename, 0, sizeof(filename));
			int pointer = 0;
			strcat(args, "\n");
			for(int i = 0; i < sizeof(args); i++) {
				if(args[i] == '\n' || args[i] == ',') {
					char copyfilename[DIRPATHLEN];
					strcpy(copyfilename, filename);
					// printf(">[%s]\n", filename);
					if(handlePut(sock, PUT, copyfilename, filename, _DEBUG) < 0) {
						printf("ERROR:: MPUT() failed on [%s]\n",filename);
						break;
					}
					memset(filename, 0, sizeof(filename));
					pointer = 0;
				}
				else if(!isspace(args[i])) {
					filename[pointer] = args[i];
					pointer++;
				}
			}
		}



		else if(strcmp(cmd, QUIT) == 0) {
		// 11. quit
			if(_DEBUG) { printf(">> QUIT\n"); }
			printf("Exiting...\n");
			close(sock);
			break;
		}




		else {
			if(_DEBUG) { printf(">> INVALID\n"); }
			printf("Invalid command.\n");
			continue;
		}


		memset(cmd, 0, sizeof(cmd));
		memset(args, 0, sizeof(args));
		memset(entered_cmd, 0, sizeof(entered_cmd));
	}



}