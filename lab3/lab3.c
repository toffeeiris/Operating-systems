#define _GNU_SOURCE //для struct sigaction sa

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

//int getpagesize(void) - получить размер страницы памяти в системе и передавать его

typedef struct 
{
    int pipefd[2];
}p_data;

typedef struct 
{
    int flag;
    p_data *pipe; 
}targs;

typedef struct 
{
    targs *arg1;
    targs *arg2;
    pthread_t *ind1;
    pthread_t *ind2;
    p_data curr_pipe;
}final;

final *global_final = NULL;

void sig_handler(int signo)
{
    printf("\nget SIGINT; %d\n", signo);
    
    if (global_final)
    {
        global_final->arg1->flag = 1;
        global_final->arg2->flag = 1;
        pthread_join(*global_final->ind1, NULL);
        pthread_join(*global_final->ind2, NULL);

        close(global_final->curr_pipe.pipefd[1]);
        close(global_final->curr_pipe.pipefd[0]);
        printf("Программа завершила работу\n");
    }
    
    exit(0);
}

void* proc_write (void* arg)
{
    printf("Поток записи начал работу\n");
    targs *args = (targs*) arg;
    int buf = getpagesize();
    
    while(args->flag == 0)
    {
        ssize_t rv = write(args->pipe->pipefd[1], &buf, sizeof(buf));

        if (rv == -1)
        {
            perror("Ошибка записи в канал!\n");
            break;
        }
        else
        {
            printf("Полученные данные: %d\n", buf);
        }
        sleep(1);
    }
    printf("Поток записи закончил работу\n");
    close(args->pipe->pipefd[1]);
    return NULL;
}

void* proc_read (void* arg)
{
    printf("Поток чтения начал работу\n");
    targs *args = (targs*) arg;
    int buf;
    
    while(args->flag == 0)
    {
        memset(&buf, 0, sizeof(buf)); 
        ssize_t rv = read(args->pipe->pipefd[0], &buf, sizeof(buf));

        if (rv == -1)
        {
            if (errno == EAGAIN)
            {
                printf("Канал пуст!\n");
                sleep(1);
            }
            else
            {
                perror("Ошибка чтения из канала!\n");
                break;
            }
        }
        else if (rv == 0)
        {
            printf("Канал закрыт!\n");
            sleep(1);
        }
        else
        {
            printf("Принятые данные:  %d\n", buf);
        }
    }
    printf("Поток чтения закончил работу\n");
    close(args->pipe->pipefd[0]);
    return NULL;
}

int main(int argc, char* argv[])
{
    printf("Программа начала работу\n");
    if (argc != 2)
    {
        fprintf(stderr, "Нужно выбрать один из режимов!\n", argv[0]);
        fprintf(stderr, "1 - pipe()\n2 - pipe2()\n3 - pipe() и fcntl()\n");
        exit(EXIT_FAILURE);
    }
    
    pthread_t ind1, ind2;
    targs arg1, arg2;
    p_data curr_pipe;
    arg1.flag = 0;
    arg1.pipe = &curr_pipe;
    arg2.flag = 0;
    arg2.pipe = &curr_pipe;

    final curr;
    curr.arg1 = &arg1;
    curr.arg2 = &arg2;
    curr.ind1 = &ind1;
    curr.ind2 = &ind2;
    curr.curr_pipe = curr_pipe;
    global_final = &curr;

    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = (void (*)(int))sig_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);  

    int mode = atoi(argv[1]);
    switch(mode)
    {
        case 1:
            if (pipe(curr_pipe.pipefd) == -1)
            {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            break;
        case 2:
            if (pipe2(curr_pipe.pipefd, O_NONBLOCK) == -1)
            {
                perror("pipe2");
                exit(EXIT_FAILURE);
            }
            break;
        case 3:
            if (pipe(curr_pipe.pipefd) == -1)
            {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            fcntl(curr_pipe.pipefd[0], F_SETFL, O_NONBLOCK);
            fcntl(curr_pipe.pipefd[1], F_SETFL, O_NONBLOCK);
            break;
        default:
            fprintf(stderr, "Некорректный ввод! Используйте 1, 2 или 3.\n");
            exit(EXIT_FAILURE);
    }


    pthread_create(&ind1, NULL, proc_write, &arg1);
    pthread_create(&ind2, NULL, proc_read, &arg2);
    printf("Программа ждет нажатия клавиши\n");
    getchar();
    printf("Клавиша нажата\n");
    arg1.flag = 1;
    arg2.flag = 1;
    pthread_join(ind1, NULL);
    pthread_join(ind2, NULL);

    printf("Программа завершила работу\n");
    return 0;
}