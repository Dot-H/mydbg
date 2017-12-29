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
        fprintf(stderr, "%d terminates by signal %s\n", proc->pid,
                strsignal(WSTOPSIG(proc->status)));

    } else if (WIFSTOPPED(proc->status)) {
        if (WSTOPSIG(proc->status) & 0x80) {
//            fprintf(stderr, "%d hit syscall\n", proc->pid);
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


    if (proc->siginfo.si_signo == SIGTRAP && dinfos->dflt_pid) // Run test
        if (bp_hit(dinfos, proc) == -1)
            do_continue(dinfos, NULL);
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
        goto err_print_errno;

    return pid;

err_empty_dinfos:
    empty_debug_infos(dinfos);
err_print_errno:
    warn("Failed to trace %s", dinfos->args[0]);
    if (pid == 0)
        exit(1);
    return -1;
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

//    printf("peeked %lx\n", saved_data);

    opcode &= 0xff;
    long new_data = ((saved_data & ~(0xff)) | opcode);
    if (ptrace(PTRACE_POKETEXT, pid, addr, new_data) == -1) {
        warn("Failed to POKETEXT %lx in %d at %p", new_data, pid, addr);
        return -1;
    }

//    printf("poked %lx\n", new_data);

    return saved_data & 0xff;
}
