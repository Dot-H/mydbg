#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

#include "my_dbg.h"

static int print_exit(pid_t pid, int status)
{
    int ret;
    if (WIFEXITED(status)){
        ret = WEXITSTATUS(status);
        fprintf(stderr, "%d exited normally with code %d\n", pid, ret);
    }
    else if (WIFSIGNALED(status)){
        ret =  WTERMSIG(status);
        fprintf(stderr, "%d terminates by signal %d\n", pid, ret);
    }
    else if (WIFSTOPPED(status)){
        ret = WSTOPSIG(status);
        fprintf(stderr, "%d stopped by signal %d\n", pid, ret);
    }
    else if (WIFCONTINUED(status)){
        ret = SIGCONT;
        fprintf(stderr, "%d continued\n", pid);
    }
    else{
        ret = status;
        fprintf(stderr, "%d has received an unknown signal. status: %d\n",
                pid, ret);
    }

    return ret;
}

int trace_binary(struct debug_infos *dinfos)
{
    if (!dinfos->melf.elf)
        return -1;

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
    return print_exit(pid, status);

err_empty_dinfos:
    empty_debug_infos(dinfos);
err_print_errno:
    warn("Failed to trace %s", dinfos->args[0]);
    if (pid == 0)
        exit(1);
    return -1;
}
