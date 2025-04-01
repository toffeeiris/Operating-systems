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
            exit(EXIT_FAILURE);
        }
        else
        {
            item  = malloc(sizeof(struct entry));
            item->data = buffer;
            printf("client: ip = %s; port = %d", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
        }
    }
}

void* func_transfer(void *arg)
{
    info *args = (info*) arg;
    int buffer = 0;
    struct entry *item;
    while(args->flag_transfer == 0)
    {
        item = STAILQ_FIRST(&head);

    }
}

int main()
{
    info curr;
    curr.sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(7000);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");


}