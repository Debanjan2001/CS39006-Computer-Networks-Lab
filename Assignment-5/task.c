/**
 * @file task.c
 * @author DEBANJAN SAHA + PRITKUMAR GODHANI
 * @brief 19CS30014 + 19CS10048
 * @version 0.1
 * @date 2022-03-24
 * @copyright Copyright (c) 2022
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <time.h>

#define DEST_PORT 32164
#define SRC_PORT 8080

#define TIMEOUT_SEC 1

#define MAX_BUF_LEN 256

#define DATAGRAM_SIZE 4096

#define TTL_LIMIT 16

#define PAYLOAD_SIZE 52

#define IPHDR_UNIQUE_ID 30014 

void generate_payload(char *data, int len) {
	for (int i = 0; i < len; i++) {
		data[i] = (char)('a' + rand() % 26);
	}
}

void cleanup(int S1, int S2) {
	close(S1);
	close(S2);
}

double get_timedelta(clock_t start, clock_t end){
	double timedelta = (((double) (end - start)) / CLOCKS_PER_SEC)*1000 ;
	return timedelta; 
}


/**
 * @brief
 * https://www.binarytides.com/raw-udp-sockets-c-linux/
 */

int main(int argc, char *argv[]) {

	int debug_mode = 0;
	for(int i=1;i<argc;i++){
		if( strcmp(argv[i],"-d") == 0 || strcmp(argv[i],"--debug") == 0){
			debug_mode = 1;
			break;
		} 
	}

	/**
	 * Step-1
	 */
	if (argc < 2) {
		fprintf(stderr, "Argument Error: Destination Domain Name Not Found.\n");
		exit(EXIT_FAILURE);
	}
	char *dest_domain_name = argv[1];
	
	// DEBUG
	(debug_mode && printf("dest domain=%s\n", dest_domain_name));

	struct hostent *dest_info;
	struct in_addr **dest_addr_list;

	if ( (dest_info = gethostbyname(dest_domain_name)) == NULL) {
		herror("Unable to resolve Destination IP Address");
		exit(EXIT_FAILURE);
	}

	dest_addr_list = (struct in_addr **)(dest_info->h_addr_list);
	char* dest_ip = inet_ntoa(*dest_addr_list[0]);

	// DEBUG
	(debug_mode && printf("IP = %s\n", dest_ip));


	/**
	 * Step-2
	 */
	int S1, S2;
	struct sockaddr_in source_addr;
	struct sockaddr_in receiver_addr;
	socklen_t source_addr_len;
	socklen_t receiver_addr_len;

	S1 = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
	if (S1 < 0) {
		perror("Unable to create Raw Socket S1");
	}

	S2 = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (S2 < 0) {
		perror("Unable to create Raw Socket S2");
	}

	u_int32_t dest_addr = inet_addr(dest_ip);

	source_addr.sin_family = AF_INET;
	source_addr.sin_port = htons( SRC_PORT );
	source_addr.sin_addr.s_addr = INADDR_ANY;
	source_addr_len = sizeof(source_addr);

	receiver_addr.sin_family = AF_INET;
	receiver_addr.sin_port = htons( DEST_PORT );
	receiver_addr.sin_addr.s_addr = inet_addr(dest_ip);
	receiver_addr_len = sizeof(receiver_addr);


	if (bind(S1, (struct sockaddr *)&source_addr, source_addr_len ) < 0) {
		perror("Unable to Bind S1 to localhost");
		cleanup(S1, S2);
		exit(EXIT_FAILURE);
	}

	if (bind(S2, (struct sockaddr *)&source_addr, source_addr_len ) < 0) {
		perror("Unable to Bind S2 to localhost");
		cleanup(S1, S2);
		exit(EXIT_FAILURE);
	}

	/**
	 * Step-3
	 */
	const int on = 1;
	if (setsockopt(S1, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
		perror("Unable to set IP_HDRINCL on socket S1");
		cleanup(S1, S2);
		exit(EXIT_FAILURE);
	}


	// MyTraceRoute
	printf("mytraceroute to %s (%s), %d hops max, %d byte payload\n", dest_domain_name, dest_ip, TTL_LIMIT, PAYLOAD_SIZE);


	/**
	 * Step-4
	 * Works:
	 *  - Running in a loop
	 */
	int TTL = 1;
	int num_repeats = 0;

	fd_set readfds;
	int numfd_max, ready_fd;

	while(TTL <= TTL_LIMIT) {
		
		// DEBUG
		(debug_mode && printf("TTL = %d\n", TTL));

		char datagram[DATAGRAM_SIZE];
		bzero(datagram, sizeof(datagram));

		struct iphdr *iph = (struct iphdr *)datagram;
		struct udphdr *udph = (struct udphdr *)(datagram + sizeof(struct iphdr));

		char *datagram_payload_dump_site = datagram + sizeof(struct iphdr) + sizeof(struct udphdr);
		char payload[PAYLOAD_SIZE];
		generate_payload(payload, PAYLOAD_SIZE);
		strcpy(datagram_payload_dump_site, payload);

		//DEBUG
		(debug_mode && printf("Data = %s\n",datagram_payload_dump_site));

		// IP hdr
		iph->ihl = 5;
		iph->version = 4;
		iph->tos = 0;
		iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(datagram_payload_dump_site);
		iph->id = htons(IPHDR_UNIQUE_ID);
		iph->ttl = TTL;
		iph->protocol = IPPROTO_UDP;
		iph->saddr = 0;
		iph->daddr = dest_addr;
		iph->check = 0;

		// UDP hdr
		udph->source = htons(SRC_PORT);
		udph->dest = htons(DEST_PORT);
		udph->len = htons(sizeof(struct udphdr) + strlen(datagram_payload_dump_site));
		udph->check = 0;

		clock_t send_time = clock();

		int status = sendto(S1, datagram, iph->tot_len, 0, (struct sockaddr *)&receiver_addr, sizeof(receiver_addr));
		if (status < 0) {
			perror("Error during sendto() using Socket S1");
			continue;
		}

		num_repeats += 1;

		/**
		 * Step-5
		 */

		char buffer[MAX_BUF_LEN];
		

		struct timeval timeout;
		timeout.tv_sec = TIMEOUT_SEC;
		timeout.tv_usec = 0;

		while (1) {
			
			// clear the set ahead of time
			FD_ZERO(&readfds);

			FD_SET(S2, &readfds);

			numfd_max = S2 + 1;

			ready_fd = select(numfd_max, &readfds, 0, 0, &timeout);

			
			if (ready_fd < 0) {
				perror("Error during select() call for receiving ICMP msg");
				cleanup(S1, S2);
				exit(EXIT_FAILURE);
			}

			/**
			 * Step-6
			 */

			if (FD_ISSET(S2, &readfds)) {
				
				bzero(buffer, sizeof(buffer));
				int recv_len = recvfrom(S2, buffer, sizeof(buffer), 0, (struct sockaddr *)&source_addr, &source_addr_len);
				
				clock_t recv_time = clock();
				double timedelta = get_timedelta(send_time, recv_time);
				
				// DEBUG
				(debug_mode && printf("S2 has something to share 😁\n"));

				if (recv_len <= 0) {
					// Do something
					(debug_mode && printf("Nothing came out of recvfrom 😐"));
					continue;
				}

				struct iphdr *ip_reply = ((struct iphdr *)buffer);
				struct icmphdr *icmp_reply = ((struct icmphdr *)(buffer + sizeof(struct iphdr)));

				struct in_addr iph_addr;
				iph_addr.s_addr = ip_reply->saddr;

				if (ip_reply->protocol == 1) {

					if (icmp_reply->type == 3) {

						if (iph->daddr == ip_reply->saddr) {
							// print time and all stuff.
							printf("%-10d %-15s %10f ms\n", TTL, inet_ntoa(iph_addr), timedelta);
							cleanup(S1, S2);
							exit(EXIT_SUCCESS);
						} else {
							fprintf(stderr, "Could this be an attack from a hacker/bot?");
							cleanup(S1, S2);
							exit(EXIT_FAILURE);
						}

					} else if (icmp_reply->type == 11) {
						// print and stuff
						printf("%-10d %-15s %10f ms\n", TTL, inet_ntoa(iph_addr), timedelta);

						TTL += 1;
						num_repeats = 0;

						// Breakout of Step-5 Loop
						break;
						// Repeat from step-4
					} 

				} else {
					// go back to wait on select  with remaining time
					int rem_time_usec = 1000000 - timedelta * 1000;
					timeout.tv_sec = 0;
					timeout.tv_usec = rem_time_usec;
					continue;
				}

			} else {
				// increase ttl
				// on max 3 repeats
				if(num_repeats == 3){
					
					printf("%-10d %-15s %10s\n", TTL, "*", "*");

					TTL += 1;
					num_repeats = 0;
				}

				// Breakout of Step-5 Loop
				break;
				// Repeat from step-4
			}
		}
	}

	cleanup(S1, S2);

	return 0;
}