#include "linux/limits.h"
#include <unistd.h>
#include <stdio.h>
#include "LineParser.h"
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#define INPUT_MAX_SIZE 48
#define EXECUTION_FAILED 1
#define BUFFER_SIZE 12

void execute(cmdLine *pCmdLine, int debug)
{
    int returnVal;
    int err = 0;
    pid_t pid = 0;
    int status;

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

    else if ((pid = fork()) == 0)
    {
        if ((returnVal = execv(pCmdLine->arguments[0], pCmdLine->arguments)) < 0)
        {
            perror("couln't execute");
            freeCmdLines(pCmdLine);
            _exit(EXECUTION_FAILED);
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
            return 1;
        }

        fprintf(stdout, "%s\n", returnVal);

        fgets(input, INPUT_MAX_SIZE, stdin);

        if (strncmp(input, "quit", 4) == 0)
        {
            freeCmdLines(cmdL);
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