#define _REENTRANT //for CLOCK_REALTIME 

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <time.h>

typedef struct 
{
    int flag;
    char sym;
    pthread_mutex_t *mutex;
}targs;

typedef struct 
{
    targs *arg1;
    targs *arg2;
    pthread_t *ind1;
    pthread_t *ind2;
}all_data;

void sig_handler(int signo, all_data *curr)
{
    printf("\nget SIGINT; %d\n", signo);
    
    curr->arg1->flag = 1;
    curr->arg2->flag = 1;
    pthread_join(*curr->ind1, NULL);
    pthread_join(*curr->ind2, NULL);
    printf("Программа № 3 завершила работу\n");

    exit(0);
}

void* proc1(void *arg)
{
    printf("Поток 1 начал работу\n");
    targs *args = (targs*) arg;
    struct timespec tp;
    while(args->flag == 0)
    {
        int counter = 0;
        
        clock_gettime(CLOCK_REALTIME, &tp);
        tp.tv_sec += 1;
        //Вход в КУ
        if (pthread_mutex_timedlock(args->mutex, &tp) == 0)
        {
            while (counter++ <= 10 && args->flag == 0)
            {
                putchar(args->sym);
                fflush(stdout);
                sleep(1);
            }
            pthread_mutex_unlock(args->mutex);
            //Выход из КУ
            sleep(1);
        }
        else
        {
            continue;
        }
    }
    printf("Поток 1 закончил работу\n");
    pthread_exit((void*)1);
}

void* proc2(void *arg)
{
    printf("Поток 2 начал работу\n");
    targs *args = (targs*) arg;
    struct timespec tp;
    while(args->flag == 0)
    {
        int counter = 0;
        
        clock_gettime(CLOCK_REALTIME, &tp);
        tp.tv_sec += 1;
        //Вход в КУ
        if (pthread_mutex_timedlock(args->mutex, &tp) == 0)
        {
            while (counter++ <= 10 && args->flag == 0)
            {
                putchar(args->sym);
                fflush(stdout);
                sleep(1);
            }
            pthread_mutex_unlock(args->mutex);
            //Выход из КУ
            sleep(1);
        }
        else
        {
            continue;
        }
    }
    printf("Поток 2 закончил работу\n");
    pthread_exit((void*)2);
}

int main()
{
    printf("Программа № 3 начала работу\n");
    pthread_t ind1, ind2;
    targs arg1, arg2;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    arg1.flag = 0;
    arg1.sym = '1';
    arg1.mutex = &mutex;
    arg2.flag = 0;
    arg2.sym = '2';
    arg2.mutex = &mutex;
    int *exitcode1, *exitcode2;

    struct sigaction sa;
    all_data curr;
    sa.sa_flags = 0;
    sa.sa_handler = (void (*)(int))sig_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);    

    pthread_create(&ind1, NULL, proc1, &arg1);
    pthread_create(&ind2, NULL, proc2, &arg2);
    printf("Программа ждет нажатия клавиши\n");
    getchar();
    printf("Клавиша нажата\n");
    arg1.flag = 1;
    arg2.flag = 1;
    pthread_join(ind1, (void**)&exitcode1);
    printf("exitcode1 = %p\n", exitcode1);
    pthread_join(ind2, (void**)&exitcode2);
    printf("exitcode2 = %p\n", exitcode2);

    pthread_mutex_destroy(&mutex);
    printf("Программа № 3 завершила работу\n");
    return 0;
}
