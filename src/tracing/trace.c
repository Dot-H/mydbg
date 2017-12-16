#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "my_dbg.h"

static void print_exit(pid_t pid, int status)
{
    if (WIFEXITED(status)){
        fprintf(stderr, "%d exited normally with code %d\n",
                pid, WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status)){
        fprintf(stderr, "%d terminates by signal %d\n", pid, WTERMSIG(status));
    }
    else if (WIFSTOPPED(status)){
        fprintf(stderr, "%d stopped by signal %d\n", pid, WSTOPSIG(status));
    }
    else if (WIFCONTINUED(status)){
        fprintf(stderr, "%d continued\n", pid);
    }
    else{
        fprintf(stderr, "%d has received an unknown signal. status: %d\n",
                pid, status);
    }
}

void trace_binary(struct debug_infos *dinfos)
{
    if (!dinfos->melf.elf)
        return;

    pid_t pid = fork();
    if (pid == -1)
        goto err_empty_dinfos;

    if (pid == 0) {
        if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1)
            goto err_print_errno;

        printf("Starting program: %s\n", dinfos->args[0]);
        execvp(dinfos->args[0], dinfos->args);
        goto err_print_errno;
    }

    int status = -1;
    waitpid(pid, &status, 0);
    print_exit(pid, status);

    return;

err_empty_dinfos:
    empty_debug_infos(dinfos);
err_print_errno:
    warn("Failed to trace %s", dinfos->args[0]);
    if (pid == 0)
        exit(1);
}
