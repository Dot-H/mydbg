#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>

#include "commands.h"
#include "my_dbg.h"
#include "breakpoint.h"
#include "dproc.h"
#include "trace.h"

static struct dproc *get_proc(struct debug_infos *dinfos,
                              char *args[], int argsc)
{
    pid_t pid = dinfos->dflt_pid;
    if (argsc == 2) {
        pid = arg_to_long(args[1], 10);
        if (pid == -1)
            return NULL;
    }

    struct dproc *proc = dproc_htable_get(pid, dinfos->dproc_table);
    if (!proc)
        fprintf(stderr, "Could not find process %d\n", pid);

    return proc;
}


int do_continue(struct debug_infos *dinfos, char *args[])
{
    if (!is_running(dinfos))
        return -1;

    int argsc = check_params(args, 1, 2);
    if (argsc == -1)
        return -1;

    struct dproc *proc = get_proc(dinfos, args, argsc);
    if (!proc)
        return -1;

    int ret = -1;
    if ((ret = ptrace(PTRACE_CONT, proc->pid, 0, 0) == -1))
        warn("Could not resume the execution of %d", proc->pid);
    else
        wait_tracee(dinfos, proc);

    return ret;
}

shell_cmd(continue, do_continue, "Continue the execution of a process");
