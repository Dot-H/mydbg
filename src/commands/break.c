#include <stdlib.h>
#include <stdio.h>

#include "commands.h"
#include "my_dbg.h"
#include "breakpoint.h"
#include "dproc.h"
#include "trace.h"

int do_break(struct debug_infos *dinfos, char *args[])
{
    if (!is_running(dinfos))
        return -1;

    int argsc = check_params(args, 1, 2);
    if (argsc == -1)
        return -1;

    enum bp_type btype = (args[0][0] == 't') ? BP_TEMPORARY : BP_CLASSIC;

    long bp_addr = get_addr(dinfos->dflt_pid, args, argsc, 1);
    if (bp_addr == -1)
        return -1;

    struct breakpoint *bp = bp_creat(btype);
    return bp_set(dinfos, bp, (void *)bp_addr, dinfos->dflt_pid);
}

shell_cmd(break, do_break, "Put a breakpoint on the address given in argument");
shell_cmd(tbreak, do_break, "Put a temporary breakpoint on the address given \
 in argument");
