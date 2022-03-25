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
#include <signal.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

#define DEST_PORT 32164
#define SRC_PORT 8080

#define DATAGRAM_SIZE 4096

void generate_payload(char *data, int len){
    for(int i=0;i<len;i++){
        data[i] = (char)('a' + rand()%26);
    }
}

unsigned short csum(){
    //
}

/**
 * @brief 
 * https://www.binarytides.com/raw-udp-sockets-c-linux/
 */

int main(int argc, char *argv[]){

    /**
     * Step-1
     */
    if(argc < 2){
        fprintf(stderr, "Argument Error: Destination Domain Name Not Found.\n");
        exit(EXIT_FAILURE);
    }
    char *dest_domain_name = argv[1];
    printf("dest domain=%s\n", dest_domain_name);

    struct hostent *dest_info;
    struct in_addr **dest_addr_list;

    if ( (dest_info = gethostbyname(dest_domain_name)) == NULL){
        herror("Unable to resolve Destination IP Address");
        exit(EXIT_FAILURE);
    }

    dest_addr_list = (struct in_addr **)(dest_info->h_addr_list);
    char* dest_ip = inet_ntoa(*dest_addr_list[0]);

    printf("IP = %s\n",dest_ip);


    /**
     * Step-2
     */
    int S1, S2;
    struct sockaddr_in self_addr;

    S1 = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if(S1 < 0){
        perror("Unable to create UDP Socket S1");
    }

    S2 = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(S2 < 0){
        perror("Unable to create UDP Socket S2");
    }

    self_addr.sin_family = AF_INET;
    self_addr.sin_addr.s_addr = INADDR_ANY;
    self_addr.sin_port = htons( SRC_PORT );


    if(bind(S1, (struct sockaddr *)&self_addr, sizeof(self_addr) ) < 0){
        perror("Unable to Bind S1 to localhost");
        exit(EXIT_FAILURE);
    }

    if(bind(S2, (struct sockaddr *)&self_addr, sizeof(self_addr) ) < 0){
        perror("Unable to Bind S2 to localhost");
        exit(EXIT_FAILURE);
    }

    /**
     * Step-3
     */
    const int on = 1;
    if(setsockopt(S1, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0){
        perror("Unable to set IP_HDRINCL on socket S1");
        close(S1);
        close(S2);
        exit(EXIT_FAILURE);
    }

    /**
     * Step-4
     * Works: 
     *  - Properly Setting the headers
     *  - Running in a loop
     */
    int TTL = 1;

    char datagram[DATAGRAM_SIZE], *data;
    bzero(datagram,sizeof(datagram));

    struct iphdr *iph = (struct iphdr *)datagram;
    struct udphdr *udph = (struct udphdr *)(datagram + sizeof(struct iphdr));

    data = datagram + sizeof(struct iphdr) + sizeof(struct udphdr);
    char payload[52];
    generate_payload(payload,52);
    strcpy(data, payload);

    // IP hdr
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(data);
    iph->id = htons(30014);
    iph->frag_off = 0;
    iph->ttl = TTL;
    iph->protocol = IPPROTO_UDP;
    iph->check=0;
    iph->saddr = inet_addr(0);
    iph->daddr = inet_addr(dest_ip);

    // IP checksun
    iph->check = csum((unsigned short *) datagram, iph->tot_len);

    // UDP hdr
    udph->source = htons(SRC_PORT);
    udph->dest = htons(DEST_PORT);
    udph->len = htons(sizeof(struct udphdr)+ strlen(data));
    udph->check = 0;


    return 0;
}