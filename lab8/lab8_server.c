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

struct entry
{
    int data;
    STAILQ_ENTRY(request) entries;
};

STAILQ_HEAD(request_queue, request) head;

typedef struct 
{
    int flag;
    pthread_t ind;
    pthread_mutex_t *mutex;
}curr_info;

typedef struct 
{
    int sock_work, sock_linten;
    struct sockaddr_in server_addr, client_addr;
    
    curr_info receive, process_transfer, connect;
    struct request *req;

}main_info;

void* func_receive(void* arg)
{
    info *args = (info*) arg;
    int buffer = 0;
    struct entry *item;
    int counter = 0;
    while(args->flag_receive == 0)
    {
        int rv = recvfrom(args->sock_work, buffer, sizeof(int), 0, (struct sockaddr*)&args->client_addr, sizeof(args->client_addr));
        if (rv < 0)
        {
            perror("recvfrom");
            sleep(1);
        }
        else
        {
            pthrad_mutex_lock(args->mutex_connect);
            item  = malloc(sizeof(struct entry));
            item->data = buffer;
            pthrad_mutex_unlock(args->mutex_connect);
            //getcockname();
            printf("Запрос #%d принят", counter);
            printf("client: ip = %s; port = %d", inet_ntoa(args->client_addr.sin_addr), ntohs(args->client_addr.sin_port));
            counter++;
        }
    }
}

void* func_transfer(void *arg)
{
    info *args = (info*) arg;
    struct entry *item;
    int counter = 0;
    while(args->flag_transfer == 0)
    {
        item = STAILQ_FIRST(&args->head);
        int buffer = getpagesize();
        int sv = sendto(args->sock_work, buffer, sizeof(buffer), 0, (const struct sockaddr*)&args->client_addr, sizeof(args->client_addr));

        if (sv < 0)
        {
            perror("sendto");
            sleep(1);
        }
        else
        {
            printf("Было передано: %d\n", buffer); //??
            printf("Ответ на запрос #%d передан", counter);
            counter++;
        }
    }
}

void* func_connect(void *arg)
{
    main_info *args = (main_info*) arg;
    struct entry *item;
    while(args->connect.flag == 0)
    {
        pthread_mutex_lock(args->mutex_connect);
        if (STAILQ_EMPTY(&args->head))
        {
            printf("Соедиенение установлено");
            //
            pthread_mutex_unlock(args->mutex_connect);
            pthread_create(&args->ind_receive, NULL, NULL, NULL);
            pthread_create(&args->ind_transfer, NULL, NULL, NULL);
            pthread_join(args->ind_connect, NULL);
        }
        else
        {
            perror("no connection");
            pthread_mutex_unlock(args->mutex_connect);
            sleep(1);
            continue;
        }
    }
}

int main()
{
    printf("Программа-сервер начала работу\n");
    main_info this;
    this.receive.flag = 0;
    this.process_transfer.flag = 0;
    this.connect.flag = 0;

    this.sock_linten = socket(AF_INET, SOCK_DGRAM, 0);
    this.server_addr.sin_port = htons(7000);
    this.server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //fncntl(this.sock_linten, F_SETFL, O_NONBLOCK);
    int bsl = bind(this.sock_linten, (const struct sockaddr*)&this.server_addr, sizeof(this.server_addr));

    if (bsl < 0)
    {
        perror("bind");
        close(this.sock_linten);
        sleep(1);
    }

    //setsockopt(this.sock_linten, SOL_SOCKET, SO_REUSEADDR, 1, sizeof(1));
    STAILQ_INIT(&head);
    pthread_create(this.connect.ind, NULL, func_connect, &this.connect);

    printf("Программа ждет нажатия клавиши\n");
    getchar();
    printf("Клавиша нажата\n");
    this.receive.flag = 1;
    this.process_transfer.flag = 1;
    this.connect.flag = 1;
    pthread_join(this.receive.ind, NULL);
    pthread_join(this.process_transfer.ind, NULL);
    pthread_join(this.connect.ind, NULL);

    shutdown(this.sock_work, 2);
    close(this.sock_linten);
    close(this.sock_work);

    printf("Программа-сервер завершила работу\n");
}