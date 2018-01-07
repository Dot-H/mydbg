#include <err.h>
#include <elf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <link.h>

#include "my_dbg.h"
#include "commands.h"
#include "dproc.h"
#include "breakpoint.h"

static void print_status(struct dproc *proc)
{
    if (WIFEXITED(proc->status)) {
        fprintf(stderr, "%d exited normally with code %d\n", proc->pid,
                WEXITSTATUS(proc->status));

    } else if (WIFCONTINUED(proc->status)) {
        fprintf(stderr, "%d continued\n", proc->pid);

    } else if (WIFSIGNALED(proc->status)) {
        fprintf(stderr, "%d terminates by signal %s", proc->pid,
                strsignal(WSTOPSIG(proc->status)));

    } else if (WIFSTOPPED(proc->status)) {
        if (WSTOPSIG(proc->status) & 0x80) {
            return; // Reported in bp_hit if interesting
        } else
            fprintf(stderr, "%d stopped by signal %s\n", proc->pid,
                    strsignal(WSTOPSIG(proc->status)));
    } else
        fprintf(stderr, "%d has received an unknown signal. proc->status: %d\n",
                proc->pid, proc->status);
}

void wait_tracee(struct debug_infos *dinfos, struct dproc *proc)
{
    waitpid(proc->pid, &proc->status, 0);

    print_status(proc);

    if (is_finished(proc))
        return;

    if (ptrace(PTRACE_GETSIGINFO, proc->pid, 0, &proc->siginfo) == -1)
        warn("Failed to recover siginfo from %d", proc->pid);


    if (dinfos->dflt_pid) {
        if (WSTOPSIG(proc->status) == (SIGTRAP | 0x80)) {
            if (bp_sys_hit(dinfos, proc) == -1)
                do_continue(dinfos, NULL);
        } else if (WSTOPSIG(proc->status) == SIGTRAP) {
            bp_hit(dinfos, proc);
        }
    }
}

int wait_attach(struct dproc *proc)
{
    while (1) {
        waitpid(proc->pid, &proc->status, 0);
        if (WIFEXITED(proc->status)) {
            fprintf(stderr, "%d exited with code %d before beeing attached\n",
                    proc->pid, WEXITSTATUS(proc->status));
                return -1;
        } else if (WIFSIGNALED(proc->status)) {
            fprintf(stderr, "%d terminates by signal %s before beeing attached\n",
                    proc->pid, strsignal(WSTOPSIG(proc->status)));
            return -1;
        }

        int sig = 0;
        if (WIFSTOPPED(proc->status)) {
            sig = WSTOPSIG(proc->status);
            if (sig == SIGSTOP)
                break;
        }

        if (ptrace(PTRACE_CONT, proc->pid, 0, sig) == -1)
            goto err_ptrace_cont;
    }

    return 0;

err_ptrace_cont:
    warn("Could not continue %d", proc->pid);
    return -1;
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
    proc->unw.ui = _UPT_create(pid);
    if (!proc->unw.ui) {
        fprintf(stderr, "_UPT_create failed\n");
        goto err_empty_dinfos;
    }


    wait_tracee(dinfos, proc);
    if (ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD
                                        | PTRACE_O_EXITKILL) == -1)
        goto err_empty_dinfos;

    return pid;

err_empty_dinfos:
    empty_debug_infos(dinfos);
err_print_errno:
    warn("Failed to trace %s", dinfos->args[0]);
    if (pid == 0)
        exit(1);
    return -1;
}

int attach_proc(struct dproc *proc)
{
    proc->is_attached = 1;
    if (ptrace(PTRACE_ATTACH, proc->pid, 0, 0) == -1) {
        warn("Could not attach to %d", proc->pid);
        return -1;
    }

    if (wait_attach(proc) == -1)
        return -1;

    proc->unw.ui = _UPT_create(proc->pid);
    if (!proc->unw.ui) {
        fprintf(stderr, "_UPT_create failed\n");
        return -1;
    }

    if (ptrace(PTRACE_SETOPTIONS, proc->pid, 0, PTRACE_O_TRACESYSGOOD) == -1) {
        warn("Could not set ptrace options of %d\n", proc->pid);
        return -1;
    }

    return 0;
}

long set_opcode(pid_t pid, long opcode, void *addr)
{
    long saved_data = ptrace(PTRACE_PEEKTEXT, pid, addr, NULL);
    if (saved_data == -1) {
        if (errno == ESRCH) // Process finished
            return 0;

        warn("Failed to PEEKTEXT %d at %p", pid, addr);
        return -1;
    }

    opcode &= 0xff;
    long new_data = ((saved_data & ~(0xff)) | opcode);
    if (ptrace(PTRACE_POKETEXT, pid, addr, new_data) == -1) {
        warn("Failed to POKETEXT %lx in %d at %p", new_data, pid, addr);
        return -1;
    }

    return saved_data & 0xff;
}
