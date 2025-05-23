#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

typedef struct 
{
    int flag;
    char sym;
}targs;


void* proc1(void *arg)
{
    printf("Поток 1 начал работу\n");
    targs *args = (targs*) arg;
    while(args->flag == 0)
    {
        putchar(args->sym);
        fflush(stdout);
        sleep(1);
    }
    printf("Поток 1 закончил работу\n");
    pthread_exit((void*)1);
}

void* proc2(void *arg)
{
    printf("Поток 2 начал работу\n");
    targs *args = (targs*) arg;
    while(args->flag == 0)
    {
        putchar(args->sym);
        fflush(stdout);
        sleep(1);
    }
    printf("Поток 2 закончил работу\n");
    pthread_exit((void*)2);
}

int main()
{
    printf("Программа начала работу\n");
    pthread_t ind1, ind2;
    targs arg1, arg2;
    arg1.flag = 0;
    arg1.sym = '1';
    arg2.flag = 0;
    arg2.sym = '2';
    int *exitcode1, *exitcode2;
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
    printf("Программа завершила работу\n");
    return 0;
}