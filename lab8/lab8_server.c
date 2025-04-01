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
    STAILQ_ENTRY(entry) entries;
};

typedef struct 
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    int flag_receive;
    int flag_transfer;
    int flag_connect;

    pthread_t ind_receive;
    pthread_t ind_transfer;
    pthread_t ind_connect;

    STAILQ_HEAD(stailhead, entry);
    struct stailhead head;
    STAILQ_INIT(&head);

}info;

void* func_receive(void* arg)
{
    info *args = (info*) arg;
    int buffer = 0;
    struct entry *item;
    while(args->flag_receive == 0)
    {
        int rv = recvfrom(args->sockfd, buffer, sizeof(int), 0, (struct sockaddr*)&args->client_addr, sizeof(args->client_addr));
        if (rv < 0)
        {
            perror("recvfrom");
            sleep(1);
        }
        else
        {
            item  = malloc(sizeof(struct entry));
            item->data = buffer;
            //printf("client: ip = %s; port = %d", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
        }
    }
}

void* func_transfer(void *arg)
{
    info *args = (info*) arg;
    struct entry *item;
    while(args->flag_transfer == 0)
    {
        item = STAILQ_FIRST(&head);
        int buffer = getpagesize();
        int sv = sendto(args->sockfd, buffer, sizeof(buffer), 0, (const struct sockaddr*)&args->client_addr, sizeof(args->client_addr));

        if (sv < 0)
        {
            perror("sendto");
            sleep(1);
        }
        else
        {
            printf("Было передано: %d\n", buffer);
        }
    }
}

void* func_connect(void *arg)
{
    info *args = (info*) arg;
    struct entry *item;
    while(args->flag_connect == 0)
    {
        if (STAILQ_EMPTY(&head))
        {
            pthread_create(&args->ind_receive, NULL, NULL, NULL);
            pthread_create(&args->ind_transfer, NULL, NULL, NULL);
            pthread_join(args->ind_connect, NULL);
        }
        else
        {
            perror("no connection");
            sleep(1);
        }
    }
}

int main()
{
    printf("Программа-сервер начала работу\n");
    info curr;
    curr.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    fncntl(curr.sockfd, F_SETFL, O_NONBLOCK);
    int bd = bind(curr.sockfd, (const struct sockaddr*)&curr.server_addr, sizeof(curr.server_addr));

    if (bd < 0)
    {
        perror("bind");
        close(curr.sockfd);
        sleep(1);
    }

    setsockopt(curr.sockfd, SOL_SOCKET, SO_REUSEADDR, 1, sizeof(1));
    
    curr.server_addr.sin_port = htons(7000);
    curr.server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Программа ждет нажатия клавиши\n");
    getchar();
    printf("Клавиша нажата\n");
    curr.flag_receive = 1;
    curr.flag_transfer = 1;
    curr.flag_connect = 1;
    pthread_join(curr.ind_receive, NULL);
    pthread_join(curr.ind_transfer, NULL);
    close(curr.sockfd);

    printf("Программа-сервер завершила работу\n");
}