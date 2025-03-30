// вариант 1 - execve()

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[], char* envp[])
{
    printf("\nДочерний процесс начал работу\n");
    printf("pid: %d, ppid: %d\n", getpid(), getppid());

    printf("Аргументы родительского процесса:\n");
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < argc; j++)
        {
            printf("%d: %s  ", j, argv[j]);
        }
        printf("\n");
        sleep(1);
    }
    
   
    printf("Переменные окружения:\n");
    for (int i = 0; envp[i] != NULL; i++) 
    {
        printf("%s  ", envp[i]);
    }

    int exitcode = 5;
    printf("\nДочерний процесс завершил работу с кодом: %d\n", exitcode);
    exit(exitcode);
    return 0;
}