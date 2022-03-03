#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <ctype.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>

#define SOCK_MRP 48
#define MAX_TABLE_SIZE 50
#define MAX_SEQ_NUM 100
#define MAX_BUFFER_SIZE 1500
#define T_SEC 2
#define T_nSEC 0
#define STX 2
#define ACK 6
/**
  * Two tables : 
  *    1) Unack'ed Message Table : 
  *        3 fields necessary : sequence no. 
  *                             destination addr-port
  *                             message_size
  *                             message
  *    
  *    2) Recv'ed Message Table :
  *        one field necessary : message
  *
  * FIFO Queue based implementation as linked list or circular list implementation
  * 
  * Message Format : [Special Char to denote normal message] [Sequence Number Field] [Message] [NULL]
  *     ACK Format : [Special Char to denote ack message   ] [Sequence Number Field] [NULL]
  */

typedef struct _unacknode {
    int seq_num;
    struct sockaddr* dest_addr;
    socklen_t dest_len;
    int msg_len;
    char* msg;
    struct _unacknode* next;
} unack_msg;

typedef struct _recvnode {
    int msg_len;
    char* msg;
} recv_msg;

typedef struct unacktable_ {
    int next_seq_num;
    unack_msg* table;
    int top;
    int size;
} unack_msg_table_t;

typedef struct recvtable_ {
    recv_msg* table;
    int top;
    int size;
} recv_msg_table_t;

typedef struct _thread_data {
    int sockfd;
}thread_data;

unack_msg_table_t* unack_msg_table;
recv_msg_table_t* recv_msg_table;
pthread_t r_tid, s_tid;
pthread_mutex_t unack_mutex;
pthread_mutex_t recv_mutex;

int r_socket(int domain, int type, int protocol) {
    if(type != SOCK_MRP) {
        perror("ERROR: socket type has to be SOCK_MRP\n");
        return -1;
    }
    
    int sockfd = socket(domain, SOCK_DGRAM, protocol);
    if(sockfd < 0) {
        return sockfd;
    }

    unack_msg_table = (unack_msg_table_t*) malloc(sizeof(unack_msg_table_t));
    unack_msg_table->next_seq_num = 0;
    unack_msg_table->table = NULL;//(unack_msg*) malloc(MAX_TABLE_SIZE * sizeof(unack_msg));
    // unack_msg_table->top = 0;
    unack_msg_table->size = 0;

    recv_msg_table = (recv_msg_table_t*) malloc(sizeof(recv_msg_table_t));
    recv_msg_table->top = 0;
    recv_msg_table->size = 0;
    recv_msg_table->table = (recv_msg*) malloc(MAX_TABLE_SIZE * sizeof(recv_msg));

    
    thread_data r_param, s_param;
    r_param.sockfd = sockfd;
    s_param.sockfd = sockfd;
    pthread_mutex_init(&unack_mutex, NULL);
    pthread_mutex_init(&recv_mutex, NULL);
    pthread_create(&r_tid, NULL, r_thread_handler, (void *)&r_param);
    pthread_create(&s_tid, NULL, s_thread_handler, (void *)&s_param);

    return sockfd;
}

int r_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    return bind(sockfd, addr, addrlen);
}

int insert_unack_entry(char* buffer, int final_msg_len, int seq_num, const struct sockaddr* dest_addr, socklen_t dest_len) {
    // create a new entry
    unack_msg* newnode = (unack_msg*) malloc(sizeof(unack_msg));

    // copy the message and seqnumbers
    newnode->next = NULL;
    newnode->seq_num = seq_num;
    newnode->msg_len = final_msg_len;
    newnode->msg = (char *) malloc(final_msg_len*sizeof(char));
    memcpy(newnode->msg, buffer, final_msg_len);
    
    // copy the dest addr and addr len
    newnode->dest_len = dest_len;
    newnode->dest_addr = (struct sockaddr *) malloc(dest_len);
    memcpy(newnode->dest_addr, dest_addr, dest_len);
    
    // add entry to the unack_msg_table
    newnode->next = unack_msg_table->table;
    unack_msg_table->table = newnode;
    unack_msg_table->size += 1;

    return 0;
}

void delete_unack_entry(int seq_num) {
    unack_msg* p = unack_msg_table->table;
    unack_msg* prev = NULL;
    while (p != NULL) {
        if(p->seq_num == seq_num) {
            break;
        }
        prev = p;
        p = p->next;
    }

    if(p == unack_msg_table->table) {
        unack_msg_table->table = unack_msg_table->table->next;
    }else {
        prev->next = p->next;
    }
    unack_msg_table->size -= 1;
    free(p->dest_addr);
    free(p->msg);
    free(p);

    return ;
}

ssize_t r_sendto(int sockfd, const void* message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len) {
    /**
     * ASSUMPTION: current sequence number should be maintained globally.
     *             since the higher level application should model only the 
     * 
     */
    int seq_num = unack_msg_table->next_seq_num;
    unack_msg_table->next_seq_num += 1;
    unack_msg_table->next_seq_num %= MAX_SEQ_NUM;

    size_t final_msg_len = length + 3;
    
    char buffer[final_msg_len];
    buffer[0] = STX;
    buffer[1] = '0' + seq_num/10;
    buffer[2] = '0' + seq_num%10;

    memcpy(&buffer[3], (const char *)message, length);

    // memcpy(&unack_msg_table->table[unack_msg_table->top].dest_addr, dest_addr, dest_len);
    // unack_msg_table->table[unack_msg_table->top].msg = (char *) malloc(final_msg_len*sizeof(char));
    // memcpy(&unack_msg_table->table[unack_msg_table->top].msg, buffer, final_msg_len);
    // unack_msg_table->table[unack_msg_table->top].msg_len = final_msg_len;
    // unack_msg_table->table[unack_msg_table->top].seq_num = seq_num;

    // unack_msg_table->top += 1;
    // unack_msg_table->size += 1;
    // unack_msg_table->top %= MAX_TABLE_SIZE;

    if(insert_unack_entry(buffer, final_msg_len, seq_num, dest_addr, dest_len) < 0) {
        perror("ERROR:: unack_msg_table is full. \n");
        return -1;
    }

    return sendto(sockfd, buffer, final_msg_len, flags, dest_addr, dest_len);
}

struct timespec recv_time_wait = {0, 4000};
struct timespec recv_time_spill;

int insert_recv_entry() {

}

void delete_recv_entry() {
    free(recv_msg_table->table[recv_msg_table->top].msg);
    recv_msg_table->size -= 1;
    recv_msg_table->top += 1;
    recv_msg_table->top %= MAX_TABLE_SIZE;
}

ssize_t r_recvfrom(int sockfd, void *restrict buffer, size_t len, int flags, struct sockaddr *restrict src_addr, socklen_t *restrict addrlen) {
    while(recv_msg_table->size <= 0) {
        nanosleep(&recv_time_wait, &recv_time_spill);
    }

    /**
     * ASSUMPTION: for now it is assumed that the provided buffer will be long enough to hold the message
     *             this behavior may need to be modified
     * 
     */
    int msg_len = recv_msg_table->table[recv_msg_table->top].msg_len;
    memcpy(buffer, recv_msg_table->table[recv_msg_table->top].msg, msg_len);
    // free(recv_msg_table->table[recv_msg_table->top].msg);
    // recv_msg_table->size -= 1;
    // recv_msg_table->top += 1;
    // recv_msg_table->top %= MAX_TABLE_SIZE;
    delete_recv_entry();

    return msg_len;
}

int r_close(int sockfd) {
    pthread_cancel(r_tid);
    pthread_cancel(s_tid);

    int recv_size = recv_msg_table->size;
    for(int i = 0; i < recv_size; i++) {
        delete_recv_entry();
    }
    free(recv_msg_table->table);
    free(recv_msg_table);
    
    unack_msg* p = unack_msg_table->table;
    while(p != NULL) {
        delete_unack_entry(p->seq_num);
        p = unack_msg_table->table;
    }
    free(unack_msg_table);

    return close(sockfd);
}

void* r_thread_handler(void* param) {
    /**
     * DOUBT: what should be the buffer_length since we dont know what length to expect
     *        correspondingly, we must there is a header that denotes ack / normal messages
     *        so splitting a message midway may lead to confusion
     *        one solution is fixing the packet length...
     *        second is using what we used last time to code the packet length as well 
     * 
     */ 
    thread_data * parameter = (thread_data *) param;
    int sockfd = parameter->sockfd;
    char buffer[MAX_BUFFER_SIZE];
    struct sockaddr_in src_addr;
    bzero(src_addr, sizeof(src_addr));
    socklen_t src_addr_len = sizeof(src_addr);
    int flags = 0;
    while(1) {
        memset(buffer, 0, sizeof(buffer));
        int char_read = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, flags,(struct sockaddr *)&src_addr, &src_addr_len);

        if(buffer[0] == STX) {
            // data packet, decode find sequence number, store in recv table
            // send ack
            int seq_num = (buffer[1]-'0')*10 + (buffer[2]-'0');
            char message[MAX_BUFFER_SIZE];
            bzero(message, sizeof(message));

            memcpy(message, &buffer[3], char_read-3);

            pthread_mutex_lock(&recv_mutex);
            insert_recv_entry(seq_num, message);
            pthread_mutex_unlock(&recv_mutex);

            char ackmessage[MAX_BUFFER_SIZE];
            bzero(ackmessage, sizeof(ackmessage));

            sprintf(ackmessage, "%c%c%c", ACK, buffer[1], buffer[2]);
            
            int flags = 0;
            sendto(sockfd, ackmessage, 3, flags, (struct sockaddr *)&src_addr, src_addr_len);

        } else if(buffer[0] == ACK) {
            // this is an ack message
            // mark up the corresponding entry and remove it from the table
            int seq_num = (buffer[1]-'0')*10 + (buffer[2]-'0');
            
            pthread_mutex_lock(&unack_mutex);
            delete_unack_entry(seq_num);
            pthread_mutex_unlock(&unack_mutex);

        }
    }

    pthread_exit(NULL);
}

void* s_thread_handler(void* param) {
    struct timespec s_thread_timeout = {T_SEC, T_nSEC};
    while(1) {
        
    }
    pthread_exit(NULL);
}