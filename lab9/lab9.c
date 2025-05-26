#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>
#include <sys/utsname.h>

#define STACK_SIZE (1024 * 1024)

static int childFunc(void *arg)
{
    sethostname("child_host", 10);
    struct utsname uts;
    uname(&uts);
    printf("child hostname: %s\n", uts.nodename);
    return 0;
}

int main()
{
    char *stack = malloc(STACK_SIZE);
    char *top = stack + STACK_SIZE;

    struct utsname uts;
    uname(&uts);
    printf("\nparent hostname: %s\n", uts.nodename);

    printf("\nС флагом CLONE_NEWUTS\n");
    pid_t child_pid1 = clone(childFunc, top, CLONE_NEWUTS | SIGCHLD, NULL);
    if (child_pid1 == -1)
    {
        perror("clone");
        exit(EXIT_FAILURE);
    }

    waitpid(child_pid1, NULL, 0);
    uname(&uts);
    printf("check parent hostname: %s\n", uts.nodename);

    printf("\nБез флага CLONE_NEWUTS\n");
    pid_t child_pid2 = clone(childFunc, top, SIGCHLD, NULL);
    if (child_pid2 == -1)
    {
        perror("clone");
        exit(EXIT_FAILURE);
    }

    waitpid(child_pid2, NULL, 0);
    uname(&uts);
    printf("check parent hostname: %s\n", uts.nodename);

    free(stack);
    return 0;
}
