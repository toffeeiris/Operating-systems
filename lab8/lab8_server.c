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
    main_info *args = (main_info*) arg;
    int buffer = 0;
    struct entry *item;
    int counter = 0;
    while(args->receive.flag == 0)
    {
        int rv = recvfrom(args->sock_work, buffer, sizeof(buffer), 0, (struct sockaddr*)&args->client_addr, sizeof(args->client_addr));
        if (rv < 0)
        {
            perror("recvfrom");
            sleep(1);
        }
        else
        {
            pthrad_mutex_lock(args->connect.mutex);
            item  = malloc(sizeof(struct entry));
            item->data = buffer;
            pthrad_mutex_unlock(args->connect.mutex);
            //getcockname();
            printf("Запрос #%d принят: %d", ++counter, buffer);
            printf("client: ip = %s; port = %d", inet_ntoa(args->client_addr.sin_addr), ntohs(args->client_addr.sin_port));
        }
    }
}

void* func_process_transfer(void *arg)
{
    main_info *args = (main_info*) arg;
    struct entry *item;
    int counter = 0;
    while(args->process_transfer.flag == 0)
    {
        item = STAILQ_FIRST(&head);
        int buffer = getpagesize();
        int sv = sendto(args->sock_work, buffer, sizeof(buffer), 0, (const struct sockaddr*)&args->client_addr, sizeof(args->client_addr));

        if (sv < 0)
        {
            perror("sendto");
            sleep(1);
            continue;
        }
        else
        {
            printf("Ответ на запрос #%d передан: %d", ++counter, buffer);
        }
    }
}

void* func_connect(void *arg)
{
    main_info *args = (main_info*) arg;
    struct entry *item;
    while(args->connect.flag == 0)
    {
        pthread_mutex_lock(args->connect.mutex);
        if (STAILQ_EMPTY(&head))
        {
            pthread_mutex_unlock(args->connect.mutex);
            pthread_create(&args->receive.ind, NULL, func_receive, &args->receive);
            pthread_create(&args->process_transfer.ind, NULL, func_process_transfer, &args->process_transfer);
            pthread_join(args->connect.ind, NULL);
        }
        else
        {
            perror("no connection");
            pthread_mutex_unlock(args->connect.mutex);
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
    pthread_create(this.connect.ind, NULL, func_connect, &this);

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