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
#include <sys/socket.h>
#include <ctype.h>

// INET + UPD + 22 вариант

struct entry
{
    char* data;
    STAILQ_ENTRY(entry) entries;
};

struct stailhead head;
STAILQ_HEAD(stailhead, entry) head;

typedef struct 
{
    int flag;
    pthread_t ind;
}curr_info;

typedef struct 
{
    int sock_work, sock_linten;
    struct sockaddr_in server_addr, client_addr;
    pthread_mutex_t mutex;
    
    curr_info receive, process_transfer, connect;

}main_info;

void* func_receive(void* arg)
{
    printf("Поток приема начал работу\n");
    main_info *args = (main_info*) arg;
    int buffer = 0;
    char msg[16];
    sprintf(msg, "%d\n", buffer);
    struct entry *item;
    while(args->receive.flag == 0)
    {
        socklen_t slen = sizeof(args->client_addr);
        int rv = recvfrom(args->sock_work, msg, sizeof(msg), 0, (struct sockaddr*)&args->client_addr, &slen);
        if (rv == -1)
        {
            perror("receive");
            sleep(1);
        }
        else
        {
            pthread_mutex_lock(&args->mutex);
            item  = malloc(sizeof(struct entry));
            item->data = msg;
            STAILQ_INSERT_TAIL(&head, item, entries);
            pthread_mutex_unlock(&args->mutex);
        }
    }
    printf("Поток приема закончил работу\n");
}

void* func_process_transfer(void *arg)
{
    printf("Поток обработки и передачи начал работу\n");
    main_info *args = (main_info*) arg;
    struct entry *head_item;
    int counter = 0;
    while(args->process_transfer.flag == 0)
    {
        pthread_mutex_lock(&args->mutex);
        head_item = STAILQ_FIRST(&head);
        STAILQ_REMOVE_HEAD(&head, entries);
        pthread_mutex_unlock(&args->mutex);
        char* curr_item = head_item->data;

        while(!isdigit(*curr_item))
        {
            curr_item++;
        }

        while(isdigit(*curr_item))
        {
            counter = counter * 10 + *curr_item - 48;
            curr_item++;
        }

        printf("Запрос #%d принят\n", counter);
        free(head_item);

        int buffer = getpagesize();
        char msg[16];
        sprintf(msg, "%d\n", buffer);

        socklen_t slen = sizeof(args->client_addr);
        int sv = sendto(args->sock_work, msg, sizeof(msg), 0, (const struct sockaddr*)&args->client_addr, slen);

        if (sv == -1)
        {
            perror("sendto");
            sleep(1);
            continue;
        }
        else
        {
            printf("Ответ на запрос #%d передан: %d\n", counter, buffer);
        }
    }
    printf("Поток обработки и передачи закончил работу\n");
}

void* func_connect(void *arg)
{
    main_info *args = (main_info*) arg;
    struct entry *item;
    while(args->connect.flag == 0)
    {
        pthread_mutex_lock(&args->mutex);
        if (STAILQ_EMPTY(&head))
        {
            pthread_mutex_unlock(&args->mutex);
            pthread_create(&args->receive.ind, NULL, func_receive, &args->receive);
            pthread_create(&args->process_transfer.ind, NULL, func_process_transfer, &args->process_transfer);
            pthread_join(args->connect.ind, NULL);
        }
        else
        {
            perror("no connection");
            pthread_mutex_unlock(&args->mutex);
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
    pthread_mutex_init(&this.mutex, NULL);

    this.sock_work = socket(AF_INET, SOCK_DGRAM, 0);
    this.server_addr.sin_port = htons(7000);
    this.server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    fcntl(this.sock_work, F_SETFL, O_NONBLOCK);
    int bsl = bind(this.sock_work, (const struct sockaddr*)&this.server_addr, sizeof(this.server_addr));

    if (bsl < 0)
    {
        perror("bind");
        sleep(1);
    }

    STAILQ_INIT(&head);
    pthread_create(&this.connect.ind, NULL, func_connect, &this);

    printf("Программа ждет нажатия клавиши\n");
    getchar();
    printf("Клавиша нажата\n");
    this.receive.flag = 1;
    this.process_transfer.flag = 1;
    this.connect.flag = 1;
    pthread_join(this.receive.ind, NULL);
    pthread_join(this.process_transfer.ind, NULL);
    pthread_join(this.connect.ind, NULL);

    close(this.sock_linten);
    close(this.sock_work);

    printf("Программа-сервер завершила работу\n");
}