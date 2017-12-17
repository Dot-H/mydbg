#include <stdio.h>

#include "commands.h"
#include "my_dbg.h"
#include "dproc.h"

int print_regs(struct dproc *proc)
{
    if (unw_init_remote(&proc->unw.c, proc->unw.as,
                        proc->unw.ui) == -1) {
        fprintf(stderr, "Error while initing remote\n");
        return -1;
    }

    return 0;
}

int info_regs(struct debug_infos *dinfos, char *args[])
{
    (void)args;
    /* TODO pid in argument */

    const char *err_msg = NULL;
    if (!dinfos->melf.elf) {
        err_msg = "No running process";
        goto err_print_msg;
    }

    struct dproc *proc = dproc_htable_get(dinfos->dflt_pid,
                                          dinfos->dproc_table);
    if (proc->siginfo.si_signo != SIGTRAP) {
        err_msg = "Process is not stopped";
        goto err_process;
    }

    printf("pid found: %d\n", proc->pid);
    return print_regs(proc);

err_process:
    fprintf(stderr, "Could no get registers of %d:", dinfos->dflt_pid); 
err_print_msg:
    fprintf(stderr, "%s\n", err_msg);
    return -1;
}

shell_cmd(info_reg, info_regs, "Print the studied process's registers");
