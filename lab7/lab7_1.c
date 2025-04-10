#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <mqueue.h>

//7.1 - POSIX (без блокировки) + вариант 22 

typedef struct
{
    int flag;
    mqd_t mqid;   
    pthread_t p_ind; 
}targs;

void* func(void* arg)
{
    targs *args = (targs*) arg;
    int buffer;
    while (args->flag == 0)
    {
        buffer = getpagesize();
        printf("Данные для записи в очередь: %d\n", buffer);
        char msg[sizeof(int)];
        sprintf(msg, "%d", buffer);
        int result = mq_send(args->mqid, msg, sizeof(int), 1);  
        if (result == -1)
        {
            perror("mq_send");
        }
        sleep(1);
    }
}

int main()
{
    printf("Программа 1 начала работу\n");
    targs curr;
    curr.flag = 0;

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 1;
    attr.mq_msgsize = 16;
    attr.mq_curmsgs = 0;

    curr.mqid = mq_open("/myqueue", O_CREAT | O_WRONLY | O_NONBLOCK, 0644, &attr); 
    pthread_create(&curr.p_ind, NULL, func, &curr);

    printf("Программа ждет нажатия клавиши\n");
    getchar();
    printf("Клавиша нажата\n");

    curr.flag = 1;
    pthread_join(curr.p_ind, NULL);
    mq_close(curr.mqid);
    mq_unlink("/myqueue");

    printf("Программа 1 завершила работу\n");
    return 0;
}

