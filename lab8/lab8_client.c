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
#include <sys/time.h>
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
    int request_counter;
    pthread_mutex_t mutex;
    int last_processed_response;
    
    curr_info transfer_reqv, receive;

}main_info;

void* func_transfer_reqv(void* arg)
{
    printf("Поток отправки запросов начал работу\n");
    main_info* args = (main_info*) arg;
    while(args->transfer_reqv.flag == 0)
    {
        pthread_mutex_lock(&args->mutex);
        int request_id = ++args->request_counter;
        pthread_mutex_unlock(&args->mutex);

        char msg[32];
        snprintf(msg, sizeof(msg), "%d", request_id);
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
            printf("Запрос #%d передан\n", request_id);

            //предотвращение повторной отправки (ожидание)
            struct timeval start, now;
            gettimeofday(&start, NULL);
            int response_received = 0;

            while(!response_received && args->transfer_reqv.flag == 0)
            {
                pthread_mutex_lock(&args->mutex);
                response_received = (args->last_processed_response >= request_id);
                pthread_mutex_unlock(&args->mutex);

                if (!response_received)
                {
                    gettimeofday(&now, NULL);
                    if ((now.tv_sec - start.tv_sec) > 2)
                    {
                        break;
                    }
                    usleep(100000);
                }
            }

            sleep(1);
        }
    }
    printf("Поток отправки запросов завершил работу\n");
}

void* func_receive(void* arg)
{
    printf("Поток обработки ответов начал работу\n");
    main_info* args = (main_info*) arg;
    while(args->receive.flag == 0)
    {
        char msg[32];
        int buffer;

        struct timeval tv = {0, 100000};
        setsockopt(args->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        socklen_t slen = sizeof(args->server_addr);
        int rv = recvfrom(args->sock, msg, sizeof(msg), 0, (struct sockaddr*)&args->server_addr, &slen);
        
        if (rv == -1)
        {
            perror("recvfrom");
            sleep(1);
        }
        else
        {
            int request_id;
            sscanf(msg, "%d:%d", &request_id, &buffer);
            pthread_mutex_lock(&args->mutex);
            if (request_id > args->last_processed_response)
            {
                args->last_processed_response = request_id;
            }
            pthread_mutex_unlock(&args->mutex);
            printf("Ответ на запрос #%d принят: %d\n", args->request_counter, buffer);
            
            sleep(1);
        }
    }
    printf("Поток обработки ответов закончил работу\n");
}

int main()
{
    printf("Программа-клиент начала работу\n");
    main_info this;
    memset(&this, 0, sizeof(this));

    this.sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (this.sock == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&this.server_addr, 0, sizeof(this.server_addr));
    pthread_mutex_init(&this.mutex, NULL);
    this.server_addr.sin_family = AF_INET;
    this.server_addr.sin_port = htons(7000);
    this.server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    fcntl(this.sock, F_SETFL, O_NONBLOCK);
    int optval = 1;
    setsockopt(this.sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    pthread_create(&this.transfer_reqv.ind, NULL, func_transfer_reqv, &this);
    pthread_create(&this.receive.ind, NULL, func_receive, &this);

    printf("Программа ждет нажатия клавиши\n");
    getchar();
    printf("Клавиша нажата\n");
    this.transfer_reqv.flag = 1;
    this.receive.flag = 1;

    pthread_join(this.transfer_reqv.ind, NULL);
    pthread_join(this.receive.ind, NULL);

    close(this.sock);
    pthread_mutex_destroy(&this.mutex);

    printf("Программа-клиент завершила работу\n");
    return 0;
}