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

#include <time.h>

#define OK 0;
#define ERR -1;

#define SOCK_MRP 48
#define MAX_TABLE_SIZE 50
#define MAX_SEQ_NUM 100
#define MAX_BUFFER_SIZE 1500
#define T_SEC 2
#define T_nSEC 0
#define STX 2
#define ACK 6
#define DROP_PROB 0.1


typedef struct _unacknode {
    int seq_num;
    struct sockaddr* dest_addr;
    socklen_t dest_len;
    int flags;
    int msg_len;
    char* msg;
    time_t msg_time;
    struct _unacknode* next;
    struct _unacknode* prev;
} unack_msg;

typedef struct _recvnode {
    int msg_len;
    char* msg;
    struct sockaddr* src_addr;
    socklen_t src_len;
    struct _recvnode* next;
} recv_msg;

typedef struct unacktable_ {
    int next_seq_num;
    unack_msg* table;
    unack_msg* tail;
    int top;
    int size;
} unack_msg_table_t;

typedef struct recvtable_ {
    recv_msg* table; // head
    // int top;
    int size;
    // Tail and Head Pointer for inserting and extracting messages 
    recv_msg* msg_in; // tail
    recv_msg* msg_out; // head
} recv_msg_table_t;

typedef struct _thread_data {
    int sockfd;
}thread_data;

unack_msg_table_t* unack_msg_table;
recv_msg_table_t* recv_msg_table;
pthread_t r_tid, s_tid;
pthread_mutex_t unack_mutex;
pthread_mutex_t recv_mutex;

int r_socket(int domain, int type, int protocol);
void delete_unack_entry(int seq_num);
int r_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
int insert_unack_entry(char* buffer, int final_msg_len, int seq_num, const struct sockaddr* dest_addr, socklen_t dest_len, int flags);
ssize_t r_sendto(int sockfd, const void* message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
struct timespec recv_time_wait;
struct timespec recv_time_spill;

int insert_recv_entry(char* msg, int msg_len, struct sockaddr* src_addr, socklen_t src_len);
void delete_recv_entry();
ssize_t r_recvfrom(int sockfd, void * buffer, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
int r_close(int sockfd);
void* r_thread_handler(void* param);
void* s_thread_handler(void* param);

int dropMessage(float p);