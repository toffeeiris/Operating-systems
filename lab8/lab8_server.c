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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <ctype.h>

// INET + UPD + 22 вариант

struct entry
{
    char* data[32];
    STAILQ_ENTRY(entry) entries;
};

STAILQ_HEAD(stailhead, entry) head;
struct stailhead head;

typedef struct 
{
    int flag;
    pthread_t ind;
}curr_info;

typedef struct 
{
    int sock;
    struct sockaddr_in server_addr, client_addr;
    pthread_mutex_t mutex;
    int request_counter;
    
    curr_info receive, process_transfer;

}main_info;

void* func_receive(void* arg)
{
    printf("Поток приема начал работу\n");
    main_info *args = (main_info*) arg;
    while(args->receive.flag == 0)
    {
        char msg[32];
        socklen_t slen = sizeof(args->client_addr);
        int rv = recvfrom(args->sock, msg, sizeof(msg), 0, (struct sockaddr*)&args->client_addr, &slen);
        if (rv == -1)
        {
            perror("receive");
            sleep(1);
        }
        else
        {
            struct entry *item  = malloc(sizeof(struct entry));
            strncpy(item->data, msg, sizeof(item->data));
            pthread_mutex_lock(&args->mutex);
            sscanf(msg, "%d", &args->request_counter);
            STAILQ_INSERT_TAIL(&head, item, entries);
            pthread_mutex_unlock(&args->mutex);
            printf("Запрос #%d принят\n", args->request_counter); 
        }
    }
    printf("Поток приема закончил работу\n");
}

void* func_process_transfer(void *arg)
{
    printf("Поток обработки и передачи начал работу\n");
    main_info *args = (main_info*) arg;
    while(args->process_transfer.flag == 0)
    {
        pthread_mutex_lock(&args->mutex);

        if (STAILQ_EMPTY(&head))
        {
            pthread_mutex_unlock(&args->mutex);
            usleep(10000);
            continue;
        }

        struct entry *item = STAILQ_FIRST(&head);
        STAILQ_REMOVE_HEAD(&head, entries);

        struct sockaddr_in client = args->client_addr;
        int request_id;
        sscanf(item->data, "%d", &request_id);

        pthread_mutex_unlock(&args->mutex);

        int buffer = getpagesize();
        char msg[32];
        snprintf(msg, sizeof(msg), "%d:%d", args->request_counter, buffer);

        socklen_t slen = sizeof(args->client_addr);
        int sv = sendto(args->sock, msg, sizeof(msg), 0, (const struct sockaddr*)&args->client_addr, slen);
        free(item);

        if (sv == -1)
        {
            perror("sendto");
            sleep(1);
            continue;
        }
        else
        {
            printf("Ответ на запрос #%d передан: %d\n", request_id, buffer);
        }
    }
    printf("Поток обработки и передачи закончил работу\n");
}


int main()
{
    printf("Программа-сервер начала работу\n");
    main_info this;
    memset(&this, 0, sizeof(this));
    pthread_mutex_init(&this.mutex, NULL);

    this.sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (this.sock == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&this.server_addr, 0, sizeof(this.server_addr));
    this.server_addr.sin_family = AF_INET;
    this.server_addr.sin_port = htons(7000);
    this.server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bsl = bind(this.sock, (const struct sockaddr*)&this.server_addr, sizeof(this.server_addr));
    if (bsl < 0)
    {
        perror("bind");
        close(this.sock);
        exit(EXIT_FAILURE);
    }

    fcntl(this.sock, F_SETFL, O_NONBLOCK);
    STAILQ_INIT(&head);
    pthread_create(&this.receive.ind, NULL, func_receive, &this);
    pthread_create(&this.process_transfer.ind, NULL, func_process_transfer, &this);

    printf("Программа ждет нажатия клавиши\n");
    getchar();
    printf("Клавиша нажата\n");
    this.receive.flag = 1;
    this.process_transfer.flag = 1;
    pthread_join(this.receive.ind, NULL);
    pthread_join(this.process_transfer.ind, NULL);

    close(this.sock);
    pthread_mutex_destroy(&this.mutex);

    printf("Программа-сервер завершила работу\n");
    return 0;
}