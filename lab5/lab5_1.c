#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/select.h>
#include <errno.h>
#include <termios.h>

//вариант 1 - функция чтения клавиши без блокировки из интернета

void set_input_mode(struct termios *or_termios)
{
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, or_termios);
    new_termios = *or_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

void reset_input_mode(const struct termios *or_termios)
{
    tcsetattr(STDIN_FILENO, TCSANOW, or_termios);
}

int kbhit()
{
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

int getch()
{
    char ch;
    if (read(STDIN_FILENO, &ch, 1) > 0)
    {
        return ch;
    }
    return -1;
}

int main()
{
    printf("Программа 1 начала работу\n");
    struct termios or_termios;
    set_input_mode(&or_termios);

    sem_t *sem = sem_open("/my_semaphore", O_CREAT, 0644, 1);
    if (sem == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen("lab5.txt", "a");
    if (file == NULL)
    {
        perror("file open");
        sem_close(sem);
        sem_unlink("/my_semaphore");
        exit(EXIT_FAILURE);
    }

    const char* ch = "1";
    while(1)
    {
        sem_wait(sem);
        //вход в КУ
        for (int i = 0; i < 10; i++)
        {
            fputs(ch, file);
            fflush(file);
            printf("1");
            fflush(stdout);
            sleep(1);
        }
        //выход из КУ
        sem_post(sem);
        sleep(1);
        //работа вне КУ
        if (kbhit())
        {
            int ch = getch();
            if (ch != -1)
            {
                printf("\n");
                break;
            }
        }
    }

    reset_input_mode(&or_termios);
    fclose(file);
    sem_close(sem);
    sem_unlink("/my_semaphore");
    printf("Программа 1 завершила работу\n");
    return 0;
}
