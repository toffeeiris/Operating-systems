#define _GNU_SOURCE //для struct sigaction sa

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <sys/stat.h>

//память - SVID, семафоры - POSIX

void reset_terminal() //восстановление терминала после ctrl + c
{
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_iflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

typedef struct
{
    int flag;
    pthread_t ind;
    sem_t *sem_write;
    sem_t *sem_read;
    int shmid;
    char *shm_addr;

}targs;

targs *final = NULL;

void sig_handler(int signo)
{
    printf("\nget SIGINT; %d\n", signo);

    if (final)
    {
        final->flag = 1;
        sem_post(final->sem_write);                            
        pthread_join(final->ind, NULL);

        sem_close(final->sem_read);
        sem_unlink("/sem_read");
        sem_close(final->sem_write);
        sem_unlink("/sem_write");
        shmdt(final->shm_addr);
        shmctl(final->shmid, IPC_RMID, NULL);
    }
    
    printf("\nПрограмма 2 завершила работу\n");
    reset_terminal();
    exit(0);
}

void* func(void* arg)
{
    targs *args = (targs*) arg;
    int data;
    while (args->flag == 0)
    {
        if (sem_wait(args->sem_write) != 0) break;
        memcpy(&data, args->shm_addr, sizeof(data));
        if (args->flag == 0)
            printf("Переданные данные: %d\n", data);
        sem_post(args->sem_read);
    }
}

int main()
{
    printf("Программа 2 начала работу\n");
        
    targs curr;
    curr.flag = 0;
    key_t key = ftok(".", 70); //указание текущего каталога

    curr.shmid = shmget(key, sizeof(int), IPC_CREAT | 0666);
    curr.shm_addr = (char*)shmat(curr.shmid, NULL, 0);
        
    curr.sem_write = sem_open("/sem_write", O_CREAT, 0644, 0);
    if (curr.sem_write == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    
    curr.sem_read = sem_open("/sem_read", O_CREAT, 0644, 0);
    if (curr.sem_read == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    pthread_create(&curr.ind, NULL, func, &curr);
    final = &curr;

    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    printf("Программа ждет нажатия клавиши\n");
    getchar();
    printf("Клавиша нажата\n");

    curr.flag = 1;
    pthread_join(curr.ind, NULL);
    
    sem_close(curr.sem_read);
    sem_unlink("/sem_read");
    sem_close(curr.sem_write);
    sem_unlink("/sem_write");
    shmdt(curr.shm_addr);
    shmctl(curr.shmid, IPC_RMID, NULL);

    printf("Программа 2 завершила работу\n");
    return 0;
}