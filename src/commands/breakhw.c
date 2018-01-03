#include <stdlib.h>
#include <stdio.h>

#include "commands.h"
#include "my_dbg.h"
#include "breakpoint.h"

int do_breakhw(struct debug_infos *dinfos, char *args[])
{
    if (!is_running(dinfos))
        return -1;

    int argsc = check_params(args, 1, 2);
    if (argsc == -1)
        return -1;

    long bp_addr = get_addr(dinfos->dflt_pid, args, argsc, 1);
    if (bp_addr == -1)
        return -1;

    struct breakpoint *bp = bp_creat(BP_HARDWARE);
    return bp_set(dinfos, bp, (void *)bp_addr, dinfos->dflt_pid);
}

shell_cmd(breakhw, do_breakhw, "Put a hardwarre breakpoint on the address \
given in argument");
