// вариант 1 - execve()

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    printf("Родительский процесс начал работу\n");
    if (argc < 4 || argc >  6)
    {
        perror("Должно быть от 3 до 5 аргументов!");
        exit(EXIT_FAILURE);
    }

    printf("pid родителя процесса: %d\n", getppid());

    printf("Аргументы командной строки родительской программы:\n");
    for (int i = 1; i < argc; i++)
    {
        printf("%d: %s\n", i, argv[i]);
    }
    
    pid_t pid = fork();
    
    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        printf("pid: %d, ppid: %d\n", getpid(), getppid());
        char *transfer[argc];
        for(int i = 0; i < argc - 1; i++)
        {
            transfer[i] = argv[i + 1];
        }
        transfer[argc - 1] = NULL;
        char *env[] = {"CUSTOM_ENV = from_parent", NULL};
        execve("./lab4_1", transfer, env);
        perror("execve\n");
        exit(EXIT_FAILURE);
    }
    else 
    {
        printf("pid: %d, child pid: %d\n", getpid(), pid);

        int status;        
        while (waitpid(pid, &status, WNOHANG) == 0)
        {
            printf("ждем\n");
            usleep(500000);
        }

        if (WIFEXITED(status))
        {
            int exitcode = WEXITSTATUS(status);
            printf("\nДочерний процесс завершился с кодом: %d\n", exitcode);
        }
    }

    printf("Родительский процесс завершил работу\n");
    return 0;
}