#include "linux/limits.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>

#define EXECUTION_FAILED 1
#define BUFFER_SIZE 12

typedef struct cmdLine
{
    char *const arguments[MAX_ARGUMENTS];
    char *const arguments2[MAX_ARGUMENTS];
    int argCount;
    int argCount2;
    int pipe;
} cmdLine;

void execute(cmdLine *pCmdLine, int debug)
{
    int returnVal;
    pid_t pid1 = 0;
    pid_t pid2 = 0;
    int pipefd[2];
    char buf;
    int status;

    if (pCmdLine->pipe)
    {
        if (pipe(pipefd) == -1)
        {
            perror("pipe");
            // freeCmdLines(pCmdLine);
            exit(EXIT_FAILURE);
        }

        if ((pid1 = fork()) == 0)
        {
            close(STDOUT_FILENO);
            dup(pipefd[1]);
            close(pipefd[1]);

            if ((returnVal = execv(pCmdLine->arguments[0], pCmdLine->arguments)) < 0)
            {
                perror("couln't execute");
                // freeCmdLines(pCmdLine);
                _exit(EXECUTION_FAILED);
            }
        }
        else if (pid1 == -1)
        {
            perror("fork");
            // freeCmdLines(pCmdLine);
            exit(EXIT_FAILURE);
        }
        else
        {
            close(pipefd[1]);
            if ((pid2 = fork()) == 0)
            {
                close(STDIN_FILENO);
                dup(pipefd[0]);
                close(pipefd[0]);
                if ((returnVal = execv(pCmdLine->arguments[0], pCmdLine->arguments)) < 0)
                {
                    perror("couln't execute");
                    // freeCmdLines(pCmdLine);
                    _exit(EXECUTION_FAILED);
                }
            }
            else if (pid2 == -1)
            {
                perror("fork");
                // freeCmdLines(pCmdLine);
                exit(EXIT_FAILURE);
            }
            else
            {
                close(pipefd[0]);
                waitpid(pid1, &status, 0);
                waitpid(pid2, &status, 0);
            }
        }
    }
    else
    {
        if ((pid1 = fork()) == 0)
        {
            if ((returnVal = execv(pCmdLine->arguments[0], pCmdLine->arguments)) < 0)
            {
                perror("couln't execute");
                // freeCmdLines(pCmdLine);
                _exit(EXECUTION_FAILED);
            }
        }
        else if (pid1 == -1)
        {
            perror("fork");
            // freeCmdLines(pCmdLine);
            exit(EXIT_FAILURE);
        }
        else
        {
            waitpid(pid1, &status, 0);
        }
    }
    if (debug == 1)
    {
        fprintf(stderr, "pid is: %d\nExecuting command is: %s\n", pid, pCmdLine->arguments[0]);
        fflush(stderr);
    }
}

static char *strClone(const char *source)
{
    char *clone = (char *)malloc(strlen(source) + 1);
    strcpy(clone, source);
    return clone;
}

cmdLine *parseCmdLines(const char *strLine)
{
    char *delimiter = " ";
    char *line, *result, *line2, *p;
    char pipeDelimiter = '|';

    if (isEmpty(strLine))
        return NULL;

    cmdLine *pCmdLine = (cmdLine *)malloc(sizeof(cmdLine));
    memset(pCmdLine, 0, sizeof(cmdLine));

    line = strClone(strLine);
    p = strtok(line, pipeDelimiter);

    if (p)
    {
        pCmdLine->pipe = 1;
        line = p;
        line2 = strtok(NULL, pipeDelimiter);
    }

    result = strtok(line, delimiter);
    while (result && pCmdLine->argCount < MAX_ARGUMENTS - 1)
    {
        ((char **)pCmdLine->arguments)[pCmdLine->argCount++] = strClone(result);
        result = strtok(NULL, delimiter);
    }
    if (pCmdLine->pipe)
    {
        result = strtok(line2, delimiter);
        while (result && pCmdLine->argCount2 < MAX_ARGUMENTS - 1)
        {
            ((char **)pCmdLine->arguments)[pCmdLine->argCount++] = strClone(result);
            result = strtok(NULL, delimiter);
        }
    }
    FREE(line);
    return pCmdLine;
}

int main(int argc, char **argv)
{
    char input[BUFSIZ];
    int i;
    int debug = 0;
    cmdLine *cmdL;

    for (i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], "-d", 2) == 0)
            debug = 1;
    }

    while (1)
    {
        fgets(input, INPUT_MAX_SIZE, stdin);

        if (strncmp(input, "quit", 4) == 0)
        {
            //  freeCmdLines(cmdL);
            break;
        }
        if ((cmdL = parseCmdLines(input)) == NULL)
        {
            fprintf(stdout, "%s", "nothing to parse\n");
            return 0;
        }
        execute(cmdL, debug);
    }
    return 0;
}
