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

void* func_transfer_reqv(void* arg)
{
    main_info* args = (main_info*) arg;
    int buffer = getpagesize();
    int counter = 0;
    char msg[sizeof(int)];
    sprintf(msg, "%d\n", buffer);
    while(args->transfer_reqv.flag == 0)
    {
        int sv = sendto(args->sock, msg, sizeof(buffer), 0, (const struct sockaddr*)&args->server_addr, sizeof(args->server_addr));
        if (sv < 0)
        {
            perror("sento");
            sleep(1);
            continue;
        }
        printf("Запрос #%d передан: %d", ++counter, buffer);
        sleep(1);
    }
}

void* func_receive(void* arg)
{
    main_info* args = (main_info*) arg;
    int buffer = 0;
    int counter = 0;
    char msg[sizeof(int)];
    while(args->receive.flag == 0)
    {
        int rv = recvfrom(args->sock, msg, sizeof(buffer), 0, (const struct sockaddr*)&args->server_addr, sizeof(args->server_addr));
        if (rv < 0)
        {
            perror("recvfrom");
            sleep(1);
        }
        else
        {
            buffer = atoi(msg);
            printf("Ответ на запрос #%d принят: %d", ++counter, buffer);
        }
    }
}

void* func_connect(void* arg)
{
    main_info* args = (main_info*) arg;
    while(args->connect.flag == 0)
    {
        pthread_create(args->transfer_reqv.ind, NULL, func_transfer_reqv, &args->connect);
        pthread_create(args->receive.ind, NULL, func_receive, &args->receive);
        pthread_join(args->connect.ind, NULL);
    }
}


int main()
{
    printf("Программа-клиент начала работу\n");
    main_info this;
    this.connect.flag = 0;
    this.transfer_reqv.flag = 0;
    this.receive.flag = 0;

    this.sock = socket(AF_INET, SOCK_DGRAM, 0);
    this.server_addr.sin_port = htons(7000);
    this.server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //fncntl(this.sock_linten, F_SETFL, O_NONBLOCK);
    int bs = bind(this.sock, (const struct sockaddr*)&this.client_addr, sizeof(this.client_addr));

    if (bs < 0)
    {
        perror("bind");
        close(this.sock);
        sleep(1);
    }

    pthread_create(this.connect.ind, NULL, func_connect, &this);
    printf("Программа ждет нажатия клавиши\n");
    getchar();
    printf("Клавиша нажата\n");
    this.transfer_reqv.flag = 1;
    this.receive.flag = 1;
    this.connect.flag = 1;

    pthread_join(this.connect.ind, NULL);
    pthread_join(this.transfer_reqv.ind, NULL);
    pthread_join(this.receive.ind, NULL);

    close(this.sock);
    printf("Программа-клиент завершила работу\n");
}