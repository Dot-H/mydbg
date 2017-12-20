#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#include "commands.h"
#include "my_dbg.h"
#include "breakpoint.h"
#include "dproc.h"
#include "trace.h"

static void *get_addr(struct debug_infos *dinfos, char *args[])
{
    void *bp_addr = NULL;
    if (!args || !args[1]) {
        struct dproc *proc = dproc_htable_get(dinfos->dflt_pid,
                                              dinfos->dproc_table);
        bp_addr = proc->siginfo.si_addr;
    }
    else {
        char *endptr = NULL;
        bp_addr = (void *)strtol(args[1], NULL, 16);
        if (endptr || errno == ERANGE)
            goto err_invalid_arg;
    }

    return bp_addr;

err_invalid_arg:
    fprintf(stderr, "Invalid argument(s)\n");
    return NULL;
}

int do_break(struct debug_infos *dinfos, char *args[])
{
    if (!dinfos->melf.elf || !dinfos->dflt_pid) {
        fprintf(stderr, "No running process\n");
        return -1;
    }

    void *bp_addr         = get_addr(dinfos, args);
    struct breakpoint *bp = bp_creat(BP_CLASSIC);
    bp->a_pid             = dinfos->dflt_pid;

    bp->sv_instr = set_opcode(bp->a_pid, BP_OPCODE, bp_addr);
    if (bp->sv_instr == -1)
        goto out_destroy_bp;

    bp->addr       = bp_addr;
    bp->is_enabled = 1;

    if (bp_htable_insert(bp, dinfos->bp_table) == -1)
    {
        fprintf(stderr, "A breakpoint is already set at %p\n",  bp_addr);
        goto out_destroy_bp;
    }

    printf("Breakpoint %d set at %p\n", bp->id, bp->addr);
    return 0;

out_destroy_bp:
    bp_destroy(bp);
    return -1;
}

shell_cmd(break, do_break, "Put a breakpoint on the address given in argument");
