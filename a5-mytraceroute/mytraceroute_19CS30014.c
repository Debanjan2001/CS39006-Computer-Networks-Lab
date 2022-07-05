/**
 * @file mytraceroute_19CS30014.c
 * @author DEBANJAN SAHA + PRITKUMAR GODHANI
 * @brief 19CS30014 + 19CS10048
 * @version 0.2
 * @date 2022-03-24
 * @copyright Copyright (c) 2022
 *
 */

/**
 * @instruction to run
 * gcc mytraceroute_19CS30014.c -o mytraceroute
 * sudo ./mytraceroute www.iitkgp.ac.in
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
#include <sys/select.h>
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
	// Returns timedelta in miliseconds
	double timedelta = (((double) (end - start)) / CLOCKS_PER_SEC)*1000 ;
	return timedelta; 
}

int max(int a, int b){
	return ((a>b)?a:b);
}


int main(int argc, char *argv[]) {

	/**
	 * Step-1 : Extracting the IP Address from domain name
	 */
	if (argc < 2) {
		fprintf(stderr, "Argument Error: Destination Domain Name Not Found.\n");
		exit(EXIT_FAILURE);
	}
	char *dest_domain_name = argv[1];
	
	struct hostent *dest_info;
	struct in_addr **dest_addr_list;

	if ( (dest_info = gethostbyname(dest_domain_name)) == NULL) {
		herror("Unable to resolve Destination IP Address");
		exit(EXIT_FAILURE);
	}

	dest_addr_list = (struct in_addr **)(dest_info->h_addr_list);
	char* dest_ip = inet_ntoa(*dest_addr_list[0]);


	/**
	 * Step-2: Creating the Raw Sockets and binding them to localhost
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
	 * Step-3 :Indicate that IP Headers will be provided manually.
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
	 * Step-4 : Sending a UDP Packet
	 */
	int TTL = 1;
	int num_repeats = 0;

	fd_set readfds;
	int numfd_max, ready_fd;

	while(TTL <= TTL_LIMIT) {
		
		char datagram[DATAGRAM_SIZE];
		bzero(datagram, sizeof(datagram));

		struct iphdr *iph = (struct iphdr *)datagram;
		struct udphdr *udph = (struct udphdr *)(datagram + sizeof(struct iphdr));

		char *datagram_payload_dump_site = datagram + sizeof(struct iphdr) + sizeof(struct udphdr);
		char payload[PAYLOAD_SIZE];
		generate_payload(payload, PAYLOAD_SIZE);
		strcpy(datagram_payload_dump_site, payload);

		// IP hdr
		iph->ihl = 5;
		iph->version = 4;
		iph->tos = 0;
		iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(datagram_payload_dump_site);
		iph->id = htons(IPHDR_UNIQUE_ID);
		iph->ttl = TTL;
		iph->protocol = IPPROTO_UDP;
		iph->frag_off = 0;
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

		/**
		 * Step-5 : Waiting on select() for receiving ICMP Packet
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
			 * Step-6 : Handling cases when select [comes/doesn't come] out with ICMP message
			 */

			if (FD_ISSET(S2, &readfds)) {
				
				bzero(buffer, sizeof(buffer));
				int recv_len = recvfrom(S2, buffer, sizeof(buffer), 0, (struct sockaddr *)&source_addr, &source_addr_len);
				
				clock_t recv_time = clock();
				double timedelta = get_timedelta(send_time, recv_time);
				
				// if (recv_len <= 0) {
				// 	// Nothing to receive? shouldn't happen. Retry
				// 	continue;
				// }

				struct iphdr *ip_reply = ((struct iphdr *)buffer);
				struct icmphdr *icmp_reply = ((struct icmphdr *)(buffer + sizeof(struct iphdr)));

				struct in_addr iph_addr;
				iph_addr.s_addr = ip_reply->saddr;

				if (ip_reply->protocol == 1) {

					if (icmp_reply->type == 3) {
						/**
						 * Step-6.b:  ICMP  Destination  Unreachable  Message
						 */

						if (iph->daddr == ip_reply->saddr) {

							printf("%-10d %-15s %10f ms\n", TTL, inet_ntoa(iph_addr), timedelta);
							cleanup(S1, S2);
							exit(EXIT_SUCCESS);

						} 

						// else {
						// 	fprintf(stderr, "Could this be an attack from a hacker/bot?");
						// 	cleanup(S1, S2);
						// 	exit(EXIT_FAILURE);
						// }

					} else if (icmp_reply->type == 11) {

						/**
						 * Transition from Step-6.c --> Step-8
						 */
						printf("%-10d %-15s %10f ms\n", TTL, inet_ntoa(iph_addr), timedelta);

						TTL += 1;
						num_repeats = 0;

						// Breakout of Step-5 Loop
						break;
						// Repeat from step-4
					} 

				} else {
					/**
					 * Step-6.d : Spurious packet, go back to wait on select with remaining time
					 */
					int rem_time_usec = max(0, 1000000 - timedelta * 1000);
					timeout.tv_sec = 0;
					timeout.tv_usec = rem_time_usec;
					continue;
				}

			} else {
				/**
				 * Step 7:  When select call times out , repeat from step-4
				 */
				num_repeats += 1;

				// increase ttl on max 3 repeats
				if(num_repeats == 3){
					
					/**
					 * Step 8: When 3 timeouts have occurred, increase TTL
					 */
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