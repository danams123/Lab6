#include "linux/limits.h"
#include <unistd.h>
#include <stdio.h>
#include "LineParser.h"
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>

#define INPUT_MAX_SIZE 48
#define EXECUTION_FAILED 1
#define HISTORY_SIZE 10

void freeHistory(char *history[], int counter)
{
    int i;
    for (i = 0; i < counter; i++)
    {
        free(history[i]);
    }
    free(history);
}

void execute(cmdLine *pCmdLine, int debug, char *history[], int counter)
{
    int returnVal;
    int err = 0;
    pid_t pid = 0;
    pid_t pid2 = 0;
    int i;
    int status;
    int fd_in;
    int fd_out;
    int pipefd[2];
    int pipeline = 0;

    if (pCmdLine->next != NULL)
    {
        pipeline = 1;
    }

    if (strncmp(pCmdLine->arguments[0], "cd", 2) == 0 && pCmdLine->argCount > 1)
    {
        if (strncmp(pCmdLine->arguments[1], "..", 2) == 0)
        {
            err = chdir("..");
        }
        else
        {
            err = chdir(pCmdLine->arguments[1]);
        }
        if (err < 0)
        {
            fprintf(stderr, "no such directory\n");
            fflush(stderr);
        }

        if (debug == 1)
        {
            fprintf(stderr, "pid is: %d\nExecuting command is: %s\n", getpid(), pCmdLine->arguments[0]);
            fflush(stderr);
        }
        return;
    }
    else if (strncmp(pCmdLine->arguments[0], "history", 8) == 0)
    {
        for (i = 0; i < counter; i++)
        {
            printf("%d) %s\n", i, history[i]);
        }

        if (debug == 1)
        {
            fprintf(stderr, "pid is: %d\nExecuting command is: %s\n", getpid(), pCmdLine->arguments[0]);
            fflush(stderr);
        }
        return;
    }

    if (pipeline)
    {
        if (pipe(pipefd) == -1)
        {
            perror("pipe");
            freeCmdLines(pCmdLine);
            freeHistory(history, counter);
            exit(EXIT_FAILURE);
        }
    }
    if ((pid = fork()) == 0)
    {
        if (pipeline)
        {
            close(STDOUT_FILENO);
            dup(pipefd[1]);
            close(pipefd[1]);
        }
        if (pCmdLine->inputRedirect != NULL)
        {
            fd_in = open(pCmdLine->inputRedirect, O_RDONLY);
            close(0);
            dup(fd_in);
            close(fd_in);
        }
        if (pCmdLine->outputRedirect != NULL)
        {
            fd_out = creat(pCmdLine->outputRedirect, 0644);
            close(1);
            dup(fd_out);
            close(fd_out);
        }
        if ((returnVal = execvp(pCmdLine->arguments[0], pCmdLine->arguments)) < 0)
        {
            perror("couldn't execute");
            freeCmdLines(pCmdLine);
            freeHistory(history, counter);

            _exit(EXECUTION_FAILED);
        }
    }
    else if (pipeline && pid != 0)
    {
        close(pipefd[1]);
        if ((pid2 = fork()) == 0)
        {
            close(STDIN_FILENO);
            dup(pipefd[0]);
            close(pipefd[0]);
            if (pCmdLine->next->inputRedirect != NULL)
            {
                fd_in = open(pCmdLine->next->inputRedirect, O_RDONLY);
                close(0);
                dup(fd_in);
                close(fd_in);
            }
            if (pCmdLine->next->outputRedirect != NULL)
            {
                fd_out = creat(pCmdLine->next->outputRedirect, 0644);
                close(1);
                dup(fd_out);
                close(fd_out);
            }
            if ((returnVal = execvp(pCmdLine->next->arguments[0], pCmdLine->next->arguments)) < 0)
            {
                perror("couldn't execute");
                freeCmdLines(pCmdLine);
                freeHistory(history, counter);
                _exit(EXECUTION_FAILED);
            }
        }
        else
        {
            close(pipefd[0]);
        }

        if (debug == 1)
        {
            fprintf(stderr, "pid is: %d\nExecuting command is: %s\n", pid, pCmdLine->arguments[0]);
            fflush(stderr);
        }
    }

    if (pCmdLine->blocking == 1)
    {
        waitpid(pid, &status, 0);
        if (pipeline)
            waitpid(pid2, &status, 0);
    }
}

int main(int argc, char **argv)
{
    char buf[PATH_MAX];
    char *input;
    char **history = (char **)malloc(sizeof(char *) * HISTORY_SIZE);
    cmdLine *cmdL;
    int i;
    int counter = 0;
    int debug = 0;

    for (i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], "-d", 2) == 0)
            debug = 1;
    }

    while (1)
    {
        char *returnVal = getcwd(buf, PATH_MAX);
        if (returnVal == NULL)
        {
            if (counter != 10)
                counter = counter + 1;
            break;
        }

        fprintf(stdout, "%s\n", returnVal);

        if (counter == 10)
        {
            free(history[0]);
            for (i = 1; i < 10; i++)
            {
                history[i - 1] = history[i];
            }
            counter = counter - 1;
        }

        history[counter] = (char *)malloc(INPUT_MAX_SIZE * sizeof(char));

        fgets(history[counter], INPUT_MAX_SIZE, stdin);

        input = strdup(history[counter]);

        if (strncmp(history[counter], "quit", 4) == 0)
        {
            if (counter != 10)
                counter = counter + 1;
            free(input);
            break;
        }

        if ((cmdL = parseCmdLines(input)) == NULL)
        {
            fprintf(stdout, "%s", "nothing to parse\n");
            if (counter != 10)
                counter = counter + 1;
            free(input);
            break;
        }

        if (counter != 10)
            counter = counter + 1;

        free(input);
        execute(cmdL, debug, history, counter);
        freeCmdLines(cmdL);
    }
    freeHistory(history, counter);
    return 0;
}
