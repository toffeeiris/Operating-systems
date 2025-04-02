#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <mqueue.h>
#include <arpa/inet.h>
#include <sys/queue.h>

// INET + UPD + 22 вариант

typedef struct 
{
    int flag;
    pthread_t ind;
    pthread_mutex_t *mutex;
}curr_info;

typedef struct 
{
    int sock;
    struct sockaddr_in server_addr, client_addr;
    
    curr_info connect, transfer_reqv, receive;
    struct request *req;

}main_info;

//3 functions

int main()
{
    printf("Программа-клиент начала работу\n");
    main_info this;
    this.connect.flag = 0;
    this.transfer_reqv.flag = 0;
    this.receive.flag = 0;

}