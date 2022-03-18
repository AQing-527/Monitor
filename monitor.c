/* Include some necessary libraries we need in this program */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <sys/resource.h>

int main(int argc, char *argv[])
{

    /* When invoking the monitor program without arguments, 
    it will terminate successfully without any output.*/
    if (argc == 1)
    {
        exit(0);
    }

    long NANO = 1000000000; /* constant that is used to convert ns to sec */
    long MICRO = 1000000;   /* constant that is used to convert ps to sec */
    int commandNum = 1;     /* the number of  commands */
    int *argNum = malloc(argc * sizeof(int));

    /* Initialize argNum, which stores the number of words in each separatable argument list. */
    for (int i = 0; i < argc; i++)
    {
        argNum[i] = 0;
    }

    char *siglist[] = {"NULL", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT", "SIGBUS",
                       "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV", "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM",
                       "", "SIGCHLD", "SIGCONT", "SIGSTOP", "SIGTSTP", "SIGTTIN", "SIGTTOU", "SIGURG",
                       "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGIO", "SIGPWR", "SIGSYS"};

    /* Obtain the number of separate argument lists and reassign the real values in argNum */
    int num = 0;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "!") == 0)
        {
            argNum[commandNum - 1] = num;
            commandNum++;
            num = 0;
        }
        else
        {
            num++;
        }
    }
    argNum[commandNum - 1] = num;

    int pipe_a[commandNum][2]; /* pipes that is used to facilitate the communication between parent process and child process */
    for (int i = 0; i < commandNum; i++)
    {
        pipe(pipe_a[i]);
    }

    int pipe_b[commandNum - 1][2]; /* pipes that is used to facilitate the communication between different child processes */
    for (int i = 0; i < commandNum - 1; i++)
    {
        pipe(pipe_b[i]);
    }

    /* allocate memory for args, which stores the separated argument lists */
    char ***args = malloc(commandNum * sizeof(char **));

    for (int i = 0; i < commandNum; i++)
    {
        args[i] = malloc(argNum[i] * sizeof(char *));
    }

    /* separate the command line arguments */
    int index = 1;
    for (int i = 0; i < commandNum; i++)
    {
        for (int j = 0; j < argNum[i]; j++)
        {
            args[i][j] = argv[index];
            index++;
        }
        index++;
    }

    pid_t pid, wpid;

    int status;                           /* the return status of child process */
    struct timespec start_time, end_time; /* variables that used to store the real time */
    struct rusage usage;

    int i;
    /* create child process(es) */
    for (i = 0; i < commandNum; i++)
    {
        pid = fork();

        /* codes for child process */
        if (pid == 0)
        {
            printf("Process with id: %d created for the command: %s\n", getpid(), args[i][0]);

            clock_gettime(CLOCK_REALTIME, &start_time);           /* record the real start time */
            write(pipe_a[i][1], &start_time, sizeof(start_time)); /* write real start time into pipe_a */

            if (i != 0)
            {
                /* read the required input of this process from pipe_b[i-1] */
                /* 进程之间交换信息的唯一途径就是传送打开的文件。 */
                dup2(pipe_b[i - 1][0], STDIN_FILENO);
            }

            if (i != commandNum - 1)
            {
                /* write the output of this process into pipe_b[i] */
                dup2(pipe_b[i][1], STDOUT_FILENO);
            }

            /* close unnecessary pipes */
            for (int k = 0; k < commandNum - 1; k++)
            {
                close(pipe_b[k][1]);
                close(pipe_b[k][0]);
            }

            /* execute a new process */
            execvp(args[i][0], args[i]);

            /* If the target program cannot be started/executed, 
            the monitor process will print out a message to indicate the problem */
            perror("exec: ");
            printf("monitor experienced an error in starting the command: %s\n", args[i][0]);
            exit(1);
        }

        else if (pid < 0)
        {
            exit(1); /* If a new process cannot be created, then exit from this program */
        }
    }

    
    /* codes for parent process */

    /* the parent process should not be affected by SIGINT */
    signal(SIGINT, SIG_IGN);

    /* close unnecessary pipes */
    for (int k = 0; k < commandNum - 1; k++)
    {
        close(pipe_b[k][1]);
        close(pipe_b[k][0]);
    }

    for (int j = 0; j < commandNum; j++)
    {
        wpid = wait4(-1, &status, 0, &usage);

        /* the parent process waits for the child process successfully */
        if (wpid > 0)
        {
            clock_gettime(CLOCK_REALTIME, &end_time);

            read(pipe_a[j][0], &start_time, sizeof(start_time));

            double real_time = end_time.tv_sec - start_time.tv_sec + (double)(end_time.tv_nsec - start_time.tv_nsec) / NANO;
            double user_time = usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec / MICRO;
            double sys_time = usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec / MICRO;

            if (!WIFEXITED(status))
            {
                printf("The command \"%s\" is interrupted by the signal number = %d (%s)\n\n", args[j][0], WTERMSIG(status), siglist[WTERMSIG(status)]);
            }

            else
            {
                printf("The command \"%s\" terminated with returned status code = %d\n\n", args[j][0], WEXITSTATUS(status));
            }

            printf("real: %.3f s, user: %.3f s, system: %.3f s\n", real_time, user_time, sys_time);
            printf("no. of page faults: %ld\n", usage.ru_minflt + usage.ru_majflt);
            printf("no. of context switches: %ld\n\n", usage.ru_nvcsw + usage.ru_nivcsw);
        }
    }
}