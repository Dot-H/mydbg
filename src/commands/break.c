#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#include "commands.h"
#include "my_dbg.h"
#include "breakpoint.h"
#include "dproc.h"
#include "trace.h"

static void *get_addr(struct debug_infos *dinfos, char *args[], int argsc)
{
    void *bp_addr = NULL;
    if (argsc == 1) {
        struct dproc *proc = dproc_htable_get(dinfos->dflt_pid,
                                              dinfos->dproc_table);
        bp_addr = proc->siginfo.si_addr;
    } else {
        long arg_addr = arg_to_long(args[1], 16);
        if (arg_addr == -1)
            return NULL;
        bp_addr = (void *)arg_addr;
    }

    return bp_addr;
}

int do_break(struct debug_infos *dinfos, char *args[])
{
    if (!is_running(dinfos))
        return -1;

    int argsc = check_params(args, 1, 2);
    if (argsc == -1)
        return -1;

    enum bp_type btype = (args[0][0] == 't') ? BP_TEMPORARY : BP_CLASSIC;

    void *bp_addr         = get_addr(dinfos, args, argsc);
    struct breakpoint *bp = bp_creat(btype);
    bp->a_pid             = dinfos->dflt_pid;

    bp->sv_instr = set_opcode(bp->a_pid, BP_OPCODE, bp_addr);
    if (bp->sv_instr == -1)
        goto out_destroy_bp;

    bp->addr  = bp_addr;
    bp->state = BP_ENABLED;

    if (bp_htable_insert(bp, dinfos->bp_table) == -1)
    {
        fprintf(stderr, "A breakpoint is already set at %p\n",  bp_addr);
        goto out_destroy_bp;
    }

    return 0;

out_destroy_bp:
    bp_destroy(bp);
    return -1;
}

shell_cmd(break, do_break, "Put a breakpoint on the address given in argument");
shell_cmd(tbreak, do_break, "Put a temporary breakpoint on the address given \
 in argument");
