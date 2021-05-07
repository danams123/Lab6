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
#define BUFFER_SIZE 12

void execute(cmdLine *pCmdLine, int debug)
{
    int returnVal;
    int err = 0;
    pid_t pid;
    pid_t pid2;
    int status;
    int fd_in;
    int fd_out;
    int pipefd[2];
    int pipeline = 0;

    if (pCmdLine->next != NULL)
    {
        pipeline = 1;
    }

    if (strncmp(pCmdLine->arguments[0], "cd", 2) == 0)
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
    }
    if (pipeline)
    {
        if (pipe(pipefd) == -1)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
    else if ((pid = fork()) == 0)
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
        if ((returnVal = execv(pCmdLine->arguments[0], pCmdLine->arguments)) < 0)
        {
            perror("couln't execute");
            freeCmdLines(pCmdLine);
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
            if ((returnVal = execv(pCmdLine->next->arguments[0], pCmdLine->next->arguments)) < 0)
            {
                perror("couln't execute");
                _exit(EXECUTION_FAILED);
            }
        }
        // else if (pid2 == -1)
        // {
        //     perror("fork");
        //     exit(EXIT_FAILURE);
        // }
        else
        {
            close(pipefd[0]);
        }
    }

    if (debug == 1)
    {
        fprintf(stderr, "pid is: %d\nExecuting command is: %s\n", pid, pCmdLine->arguments[0]);
        fflush(stderr);
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
    char input[INPUT_MAX_SIZE];
    cmdLine *cmdL;
    int i;
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
            break;
        }

        fprintf(stdout, "%s\n", returnVal);

        fgets(input, INPUT_MAX_SIZE, stdin);

        if (strncmp(input, "quit", 4) == 0)
        {
            break;
        }

        if ((cmdL = parseCmdLines(input)) == NULL)
        {
            fprintf(stdout, "%s", "nothing to parse\n");
            break;
        }
        execute(cmdL, debug);
    }
    freeCmdLines(cmdL);
    return 0;
}
