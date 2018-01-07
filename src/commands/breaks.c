#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#include "commands.h"
#include "my_dbg.h"
#include "breakpoint.h"
#include "dproc.h"
#include "trace.h"

int do_breaks(struct debug_infos *dinfos, char *args[])
{
    if (!is_traced(dinfos))
        return -1;

    int argsc = check_params(args, 2, 3);
    if (argsc == -1)
        return -1;

    struct dproc *proc = get_proc(dinfos, args, argsc, 2);
    if (!proc || !is_running(proc))
        return -1;

    long sysno = arg_to_long(args[1], 10);
    if (sysno == -1)
        return -1;

    if (sysno < 0) {
        fprintf(stderr, "Invalid syscall number: %ld\n", sysno);
        return -1;
    }

    struct breakpoint *bp = bp_creat(BP_SYSCALL);
    bp->a_pid = dinfos->dflt_pid;
    bp->sv_instr = 0;
    bp->addr  = (void *)sysno;
    bp->state = BP_ENABLED;

    if (bp_htable_insert(bp, dinfos->bp_table) == -1)
        goto err_destroy_bp;

    return 0;

err_destroy_bp:
    bp_destroy(dinfos, bp);
    return -1;
}

shell_cmd(breaks, do_breaks, "Put a breakpoint on the syscall given in argument\
 (base 10)");
