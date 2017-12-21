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

static struct dproc *get_proc(struct debug_infos *dinfos, char *args[])
{
    pid_t pid = dinfos->dflt_pid;
    if (args && args[1]) {
        char *endptr = NULL;
        pid = strtol(args[1], &endptr, 10);
        if (*endptr || errno == ERANGE)
            goto err_invalid_arg;
    }

    struct dproc *proc = dproc_htable_get(pid, dinfos->dproc_table);
    if (!proc)
        fprintf(stderr, "Could not find process %d\n", pid);

    return proc;

err_invalid_arg:
    fprintf(stderr, "Invalid argument(s)\n");
    return NULL;
}


int do_singlestep(struct debug_infos *dinfos, char *args[])
{
    if (!dinfos->melf.elf || !dinfos->dflt_pid) {
        fprintf(stderr, "No running process\n");
        return -1;
    }

    struct dproc *proc = get_proc(dinfos, args);
    if (!proc)
        return -1;

    int ret = -1;
    if ((ret = ptrace(PTRACE_SINGLESTEP, proc->pid, 0, 0) == -1))
        warn("Could not resume the execution of %d", proc->pid);
    else
        wait_tracee(dinfos, proc);

    return ret;
}

shell_cmd(singlestep, do_singlestep, "Execute a unique instruction");
