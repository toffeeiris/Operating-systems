#define _GNU_SOURCE //для struct sigaction sa

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <ev.h>

//int getpagesize(void) - получить размер страницы памяти в системе и передавать его

typedef struct 
{
    int pipefd[2];
}p_data;

typedef struct 
{
    struct ev_loop* loop;
    p_data pipe; 
    ev_timer timeout_watcher;
    ev_io io_watcher;
    ev_io stdin_watcher;
}main_data;

static void timer_cb(EV_P_ ev_timer *w, int revents)
{
    printf("Обработчик событий от таймера: запись\n");
    main_data *args = (main_data*)w->data;
    int buf = getpagesize();
    
    ssize_t rv = write(args->pipe.pipefd[1], &buf, sizeof(buf));

    if (rv == -1)
    {
        perror("Ошибка записи в канал!\n");
        ev_timer_stop(EV_A_ w);
    }
    else
    {
        printf("Полученные данные: %d\n", buf);
    }

    ev_timer_again(EV_A_ w);
}

static void io_cb(EV_P_ ev_io *w, int revents)
{
    main_data *args = (main_data*)w->data;
    printf("Обработчик событий ввода/вывода: чтение\n");
    int buf;
    ssize_t rv = read(args->pipe.pipefd[0], &buf, sizeof(buf));

    if (rv == -1)
    {
        if (errno == EAGAIN)
        {
            printf("Канал пуст!\n");
        }
        else
        {
            perror("Ошибка чтения из канала!\n");
            ev_io_stop(EV_A_ w);
        }
    }
    else if (rv == 0)
    {
        printf("Канал закрыт!\n");
        ev_io_stop(EV_A_ w);
    }
    else
    {
        printf("Принятые данные:  %d\n", buf);
    }
}

static void stdin_cb(EV_P_ ev_io *w, int revents)
{
    ev_io_stop(EV_A_ w);
    ev_break(EV_A_ EVBREAK_ALL);
}

main_data *final = NULL;

void sig_handler(int signo)
{
    printf("\nget SIGINT; %d\n", signo);
    
    if (final)
    {
        close(final->pipe.pipefd[1]);
        close(final->pipe.pipefd[0]);
    }
    
    printf("Программа завершила работу\n");
    exit(0);
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
    
    main_data curr = {0};
    curr.loop = EV_DEFAULT;
    final = &curr;

    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = (void (*)(int))sig_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);  

    int mode = atoi(argv[1]);
    switch(mode)
    {
        case 1:
            if (pipe(curr.pipe.pipefd) == -1)
            {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            break;
        case 2:
            if (pipe2(curr.pipe.pipefd, O_NONBLOCK) == -1)
            {
                perror("pipe2");
                exit(EXIT_FAILURE);
            }
            break;
        case 3:
            if (pipe(curr.pipe.pipefd) == -1)
            {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            fcntl(curr.pipe.pipefd[0], F_SETFL, O_NONBLOCK);
            fcntl(curr.pipe.pipefd[1], F_SETFL, O_NONBLOCK);
            break;
        default:
            fprintf(stderr, "Некорректный ввод! Используйте 1, 2 или 3.\n");
            exit(EXIT_FAILURE);
    }

    ev_timer_init(&curr.timeout_watcher, timer_cb, 2.0, 1.0);
    ev_io_init(&curr.io_watcher, io_cb, curr.pipe.pipefd[0], EV_READ);
    ev_io_init(&curr.stdin_watcher, stdin_cb, STDIN_FILENO, EV_READ);

    curr.timeout_watcher.data = &curr;
    curr.io_watcher.data = &curr;
    curr.stdin_watcher.data = &curr;

    ev_timer_start(curr.loop, &curr.timeout_watcher);
    ev_io_start(curr.loop, &curr.io_watcher);
    ev_io_start(curr.loop, &curr.stdin_watcher);

    printf("Программа ждет нажатия клавиши\n");
    ev_run(curr.loop, 0);

    close(curr.pipe.pipefd[0]);
    close(curr.pipe.pipefd[1]);
    final = NULL;

    printf("Клавиша нажата\n");
    printf("Программа завершила работу\n");
    return 0;
}