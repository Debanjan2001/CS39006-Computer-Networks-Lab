/**
 * @file rsocket.c
 * @author PG+DS (iitkgp)
 * @brief 
 * @version 0.1
 * @date 2022-03-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "rsocket.h"

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
    recv_msg_table->size = 0;
    recv_msg_table->table = NULL;
    recv_msg_table->msg_in = NULL;
    recv_msg_table->msg_out = NULL;

    
    thread_data *r_param = (thread_data *)malloc(sizeof(thread_data));
    thread_data *s_param = (thread_data *)malloc(sizeof(thread_data));
    r_param->sockfd = sockfd;
    s_param->sockfd = sockfd;

    pthread_mutex_init(&unack_mutex, NULL);
    pthread_mutex_init(&recv_mutex, NULL);
    pthread_create(&r_tid, NULL, r_thread_handler, (void *)r_param);
    pthread_create(&s_tid, NULL, s_thread_handler, (void *)s_param);

    return sockfd;
}

int r_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    return bind(sockfd, addr, addrlen);
}

int insert_unack_entry(char* buffer, int final_msg_len, int seq_num, const struct sockaddr* dest_addr, socklen_t dest_len, int flags) {
    // create a new entry
    unack_msg* newnode = (unack_msg*) malloc(sizeof(unack_msg));

    // copy the message and seqnumbers
    newnode->next = NULL;
    newnode->prev = NULL;
    newnode->seq_num = seq_num;
    newnode->msg_len = final_msg_len;
    newnode->msg = (char *) malloc(final_msg_len*sizeof(char));
    memcpy(newnode->msg, buffer, final_msg_len);
    newnode->flags = flags;
    
    // copy the dest addr and addr len
    newnode->dest_len = dest_len;
    newnode->dest_addr = (struct sockaddr *) malloc(dest_len);
    memcpy(newnode->dest_addr, dest_addr, dest_len);
    
    // add entry to the unack_msg_table
    newnode->next = unack_msg_table->table;
    if(unack_msg_table->table != NULL) {
        unack_msg_table->table->prev = newnode;
    } else {
        unack_msg_table->tail = newnode;
    }
    unack_msg_table->table = newnode;
    unack_msg_table->size += 1;
    // printf("Inserted into unack: %d\n", seq_num);
    return 0;
}

void delete_unack_entry(int seq_num) {
    unack_msg* p = unack_msg_table->table;
    // unack_msg* prev = NULL;
    while (p != NULL) {
        if(p->seq_num == seq_num) {
            break;
        }
        // prev = p;
        p = p->next;
    }
    if(p == NULL) return;

    if(p->prev == NULL && p->next == NULL) {
        unack_msg_table->table = NULL;
        unack_msg_table->tail = NULL;
    }else if(p->prev == NULL) {
        p->next->prev = NULL;
        unack_msg_table->table = p->next;
    }else if(p->next == NULL) {
        p->prev->next = NULL;
        unack_msg_table->tail = p->prev;
    }else{
        p->prev->next = p->next;
        p->next->prev = p->prev;
    }
    unack_msg_table->size -= 1;
    free(p->dest_addr);
    free(p->msg);
    free(p);
    // printf("Deleted %d.\n", seq_num);

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

    if(insert_unack_entry(buffer, final_msg_len, seq_num, dest_addr, dest_len, flags) < 0) {
        perror("ERROR:: unack_msg_table is full. \n");
        return -1;
    }

    sendto(sockfd, buffer, final_msg_len, flags, dest_addr, dest_len);
    return length;
}



int insert_recv_entry(char* msg, int msg_len, struct sockaddr* src_addr, socklen_t src_len) {
    // printf("Inserting message : %s \n", msg);
    // Create New Message Entry
    recv_msg *new_message = (recv_msg *)malloc(sizeof(recv_msg));
    new_message->msg = (char *) malloc(msg_len*sizeof(char));
    memcpy(new_message->msg, msg, msg_len*sizeof(char));
    new_message->msg_len = msg_len;
    new_message->next = NULL;

    new_message->src_addr = (struct sockaddr*) malloc(sizeof(struct sockaddr));
    memcpy(new_message->src_addr, src_addr, src_len);
    new_message->src_len = src_len;


    // printf("new node created.\n");
    // Insert in appropriate Position
    // Update Entry and Removal Point

    if(recv_msg_table->table == NULL) {
        // first recvd message    
        recv_msg_table->table = new_message;
        recv_msg_table->msg_in = new_message;
        recv_msg_table->msg_out = new_message;
    } else {
        // consectuive recvd messages
        recv_msg_table->msg_in->next = new_message;
        recv_msg_table->msg_in = new_message;
    }
    
    // Update table size
    recv_msg_table->size += 1;
    // printf("Inserted.\n");
    return OK;
}

void delete_recv_entry() {
    /* ++======== PREV CODE ============++*/
    // free(recv_msg_table->table[recv_msg_table->top].msg);
    // recv_msg_table->top += 1;
    // recv_msg_table->top %= MAX_TABLE_SIZE;
    /* ++======== PREV CODE ============++*/

    // Remove first message and update table
    //printf("Deleting message...\n");
    recv_msg* extracted_msg = recv_msg_table->msg_out;
    recv_msg_table->msg_out = extracted_msg->next;
    recv_msg_table->table = recv_msg_table->msg_out;

    // Release Memory
    free(extracted_msg->src_addr);
    free(extracted_msg->msg);
    free(extracted_msg);

    // Update table size
    recv_msg_table->size -= 1;
    //printf("Deleted Message.\n");
}

ssize_t r_recvfrom(int sockfd, void * buffer, size_t len, int flags, struct sockaddr * src_addr, socklen_t * addrlen) {
    recv_time_wait.tv_sec = 2;
    recv_time_wait.tv_nsec = 0;
    pthread_mutex_lock(&recv_mutex);
    int isMsg = recv_msg_table->size;
    pthread_mutex_unlock(&recv_mutex);
    while(isMsg <= 0) {
        // printf("No message yet waiting...\n");
        nanosleep(&recv_time_wait, &recv_time_spill);
        pthread_mutex_lock(&recv_mutex);
        isMsg = recv_msg_table->size;
        pthread_mutex_unlock(&recv_mutex);
    }

    /**
     * ASSUMPTION: for now it is assumed that the provided buffer will be long enough to hold the message
     *             this behavior may need to be modified
     * 
     */
    /* ++======== PREV CODE ============++*/
    // int msg_len = recv_msg_table->table[recv_msg_table->top].msg_len;
    // memcpy(buffer, recv_msg_table->table[recv_msg_table->top].msg, msg_len);
    /* ++======== PREV CODE ============++*/
    // printf("Reading message .. \n");
    pthread_mutex_lock(&recv_mutex);
    recv_msg* received_msg = recv_msg_table->msg_out;
    int msg_len = received_msg->msg_len;
    memcpy(buffer, received_msg->msg, msg_len);
    memcpy(src_addr, received_msg->src_addr, received_msg->src_len);
    *addrlen = received_msg->src_len;
    delete_recv_entry();
    pthread_mutex_unlock(&recv_mutex);
    // printf("Message read. \n");
    
    // free(recv_msg_table->table[recv_msg_table->top].msg);
    // recv_msg_table->size -= 1;
    // recv_msg_table->top += 1;
    // recv_msg_table->top %= MAX_TABLE_SIZE;

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
    bzero(&src_addr, sizeof(src_addr));
    socklen_t src_addr_len = sizeof(src_addr);
    int flags = 0;
    while(1) {
        memset(buffer, 0, sizeof(buffer));
        int char_read = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, flags, (struct sockaddr *)&src_addr, &src_addr_len);
        // printf("receved smthing..\n");
        if(dropMessage(DROP_PROB) == 1) {
            continue;
        }
        if(buffer[0] == STX) {
            // data packet, decode find sequence number, store in recv table
            // send ack
            // printf("message recved : %s \n", buffer+1);

            int seq_num = (buffer[1]-'0')*10 + (buffer[2]-'0');

            char message[MAX_BUFFER_SIZE];
            bzero(message, sizeof(message));

            memcpy(message, &buffer[3], char_read-3);

            pthread_mutex_lock(&recv_mutex);
            insert_recv_entry(message, char_read-3, (struct sockaddr *)&src_addr, src_addr_len);
            pthread_mutex_unlock(&recv_mutex);

            char ackmessage[MAX_BUFFER_SIZE];
            bzero(ackmessage, sizeof(ackmessage));

            ackmessage[0] = ACK;
            ackmessage[1] = buffer[1];
            ackmessage[2] = buffer[2];
            
            int flags = 0;
            printf("ACK SENT : %d\n", seq_num);
            sendto(sockfd, ackmessage, 3, flags, (struct sockaddr *)&src_addr, src_addr_len);

        } else if(buffer[0] == ACK) {
            // this is an ack message
            // mark up the corresponding entry and remove it from the table
            int seq_num = (buffer[1]-'0')*10 + (buffer[2]-'0');
            
            pthread_mutex_lock(&unack_mutex);
            delete_unack_entry(seq_num);
            pthread_mutex_unlock(&unack_mutex);

            printf("ACK RECVD : %d\n", seq_num);

        }
    }

    pthread_exit(NULL);
}

void* s_thread_handler(void* param) {

    thread_data* t_data = (thread_data *)param;

    // printf("Library knows sockfd = %d\n",t_data->sockfd);
 
    // Define Sleep and Timeout Period
    struct timespec s_thread_sleep_period = {T_SEC, T_nSEC};
    double msg_timeout = 2.0*T_SEC;

    // Sleep -> Check -> Retransmit -> Repeat
    while(1) {
        nanosleep(&s_thread_sleep_period, NULL);

        pthread_mutex_lock(&unack_mutex);
        unack_msg* msglist = unack_msg_table->tail;
        while(msglist != NULL){
            // pthread_mutex_lock(&unack_mutex);
            unack_msg* msg_entry = msglist;
            time_t time_now = time(NULL);
            double time_elapsed = difftime(time_now, msg_entry->msg_time);
            if(time_elapsed >= msg_timeout){
                // Update msg_entry
                msg_entry->msg_time = time_now;
                // Retransmit
                int seq_num = msg_entry->seq_num;
                size_t final_msg_len = msg_entry->msg_len;
                struct sockaddr_in destaddr;
                socklen_t destlen = msg_entry->dest_len;
                memcpy(&destaddr, msg_entry->dest_addr, destlen);
                char buffer[final_msg_len];
                memset(buffer, 0, sizeof(buffer));
                memcpy(buffer, msg_entry->msg, final_msg_len);
                msglist = msglist->prev; 
                msg_entry->msg_time = time(NULL);   
                int flags = msg_entry->flags;
                // pthread_mutex_unlock(&unack_mutex);

                // Doubt-> Normally, we terminate the string by null char.. But here, is it required?
                
                int sockfd = t_data->sockfd;

                printf("RESENDING : %d\n", seq_num);
                if(sendto(sockfd, buffer, final_msg_len, flags, (struct sockaddr *)&destaddr, destlen)<0){
                    perror("Sendto() failed\n");
                    printf("Library sees sockfd = %d\n",sockfd);
                };
            } 
            else {
                msglist = msglist->prev; 
            }
        }
        pthread_mutex_unlock(&unack_mutex);
    }
    pthread_exit(NULL);
}


int dropMessage(float p){
    float num = rand()%101;
    num /= 100;
    if( num < p){
        return 1;
    }
    return 0;
}