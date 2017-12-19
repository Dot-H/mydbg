#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

#include "my_dbg.h"
#include "dproc.h"

static void print_status(pid_t pid, int status)
{
    if (WIFEXITED(status))
        fprintf(stderr, "%d exited normally with code %d\n", pid, status);

    else if (WIFSIGNALED(status))
        fprintf(stderr, "%d terminates by signal %d\n", pid, status);

    else if (WIFSTOPPED(status))
        fprintf(stderr, "%d stopped by signal %d\n", pid, status);

    else if (WIFCONTINUED(status))
        fprintf(stderr, "%d continued\n", pid);

    else
        fprintf(stderr, "%d has received an unknown signal. status: %d\n",
                pid, status);
}

void wait_tracee(struct dproc *proc)
{
    waitpid(proc->pid, &proc->status, 0);
    print_status(proc->pid, proc->status);

    if (ptrace(PTRACE_GETSIGINFO, proc->pid, 0, &proc->siginfo) == -1)
        warn("Failed to recover siginfo from %d", proc->pid);
}

int trace_binary(struct debug_infos *dinfos, struct dproc *proc)
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

    proc->pid = pid;

    wait_tracee(proc);

    proc->unw.ui = _UPT_create(pid);
    if (!proc->unw.ui) {
        fprintf(stderr, "_UPT_create failed\n");
        goto err_empty_dinfos;
    }

    return pid;

err_empty_dinfos:
    empty_debug_infos(dinfos);
err_print_errno:
    warn("Failed to trace %s", dinfos->args[0]);
    if (pid == 0)
        exit(1);
    return -1;
}

char set_opcode(pid_t pid, long opcode, void *addr)
{
    long saved_data = ptrace(PTRACE_PEEKTEXT, pid, addr, NULL);
    if (saved_data == -1) {
        warn("Failed to PEEKTEXT %d at %p", pid, addr);
        return -1;
    }

    printf("peeked %lx\n", saved_data);

    long new_data = ((saved_data & ~(0xff)) | opcode);
    if (ptrace(PTRACE_POKETEXT, pid, addr, new_data) == -1) {
        warn("Failed to POKETEXT %lx in %d at %p", new_data, pid, addr); 
        return -1;
    }

    return saved_data & 0xff;
}
