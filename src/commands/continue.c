#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#include "commands.h"
#include "my_dbg.h"
#include "breakpoint.h"
#include "dproc.h"
#include "trace.h"

int do_continue(struct debug_infos *dinfos, char *args[])
{
    if (!is_traced(dinfos))
        return -1;

    int argsc = check_params(args, 1, 2);
    if (argsc == -1)
        return -1;

    struct dproc *proc = get_proc(dinfos, args, argsc, 1);
    if (!proc)
        return -1;

    if (bp_cont(dinfos, proc) == -1)
        return -1;

    int ret = 0;
    if ((ret = ptrace(dinfos->ptrace_req, proc->pid, 0, 0)) == -1)
        warn("Could not resume the execution of %d", proc->pid);
    else
        wait_tracee(dinfos, proc);

    return ret;
}

shell_cmd(continue, do_continue, "Continue the execution of a process");
