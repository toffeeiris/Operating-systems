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

typedef struct 
{
    int flag;
    pthread_t ind;
}curr_info;

typedef struct 
{
    int sock;
    struct sockaddr_in server_addr;
    pthread_mutex_t *mutex;
    
    curr_info connect, transfer_reqv, receive;

}main_info;

void* func_transfer_reqv(void* arg)
{
    printf("Поток отправки запросов начал работу\n");
    main_info* args = (main_info*) arg;
    int counter = 0;
    char msg[16];
    while(args->transfer_reqv.flag == 0)
    {
        printf("Запрос #%d передан\n", ++counter);
        socklen_t slen = sizeof(args->server_addr);
        int sv = sendto(args->sock, msg, sizeof(msg), 0, (const struct sockaddr*)&args->server_addr, slen);
        if (sv == -1)
        {
            perror("sento");
            sleep(1);
            continue;
        }
        else
        {
            printf("Даннные для отправки: %s\n", msg);
            sleep(1);
        }
    }
    printf("Поток отправки запросов завершил работу\n");
}

void* func_receive(void* arg)
{
    printf("Поток обработки ответов начал работу\n");
    main_info* args = (main_info*) arg;
    int buffer = 0;
    int counter = 0;
    char msg[16];
    while(args->receive.flag == 0)
    {
        socklen_t slen = sizeof(args->server_addr);
        int rv = recvfrom(args->sock, msg, sizeof(msg), 0, (struct sockaddr*)&args->server_addr, slen);
        if (rv == -1)
        {
            perror("recvfrom");
            sleep(1);
        }
        else
        {
            char* curr_item = msg;
            while(!isdigit(*curr_item))
            {
                curr_item++;
            }

            while(isdigit(*curr_item))
            {
                counter = counter * 10 + *curr_item - 48;
                curr_item++;
            }

            while(!isdigit(*curr_item))
            {
                curr_item++;
            }

            while(isdigit(*curr_item))
            {
                buffer = counter * 10 + *curr_item - 48;
                curr_item++;
            }

            printf("Ответ на запрос #%d принят: %d\n", ++counter, buffer);
        }
        sleep(1);
    }
    printf("Поток обработки ответов закончил работу\n");
}

void* func_connect(void* arg)
{
    main_info* args = (main_info*) arg;
    while(args->connect.flag == 0)
    {
        pthread_create(&args->transfer_reqv.ind, NULL, func_transfer_reqv, &args->connect);
        pthread_create(&args->receive.ind, NULL, func_receive, &args->receive);
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
    fcntl(this.sock, F_SETFL, O_NONBLOCK);
    int bs = bind(this.sock, (const struct sockaddr*)&this.server_addr, sizeof(this.server_addr));

    if (bs == -1)
    {
        perror("bind");
        sleep(1);
    }

    int optval = 1;
    setsockopt(this.sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    pthread_create(&this.connect.ind, NULL, func_connect, &this);
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