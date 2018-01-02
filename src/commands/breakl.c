#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/reg.h>

#include "commands.h"
#include "debug.h"
#include "maps.h"
#include "my_dbg.h"
#include "breakpoint.h"

static struct dw_file *get_dw_file(struct debug_infos *dinfos, long proc_addr)
{
    long addr = get_addr(dinfos->dflt_pid, NULL, 0, 1);
    if (addr == -1)
        return NULL;

    addr -= proc_addr; /* Debugs infos reffered to elf offsets */

    struct dw_file *dw = dw_htable_search_by_addr(addr, dinfos->melf.dw_table);
    if (!dw)
        fprintf(stderr, "Could not get the file from address 0x%lx\n",
                addr + proc_addr);

    return dw;
}

int do_breakl(struct debug_infos *dinfos, char *args[])
{
    if (!is_running(dinfos) || !has_debug_infos(dinfos))
        return -1;

    int argsc = check_params(args, 2, 3);
    if (argsc == -1)
        return -1;

    long lineno = arg_to_long(args[1], 10);
    if (lineno == -1)
        return -1;

    struct map *procmap = map_htable_get(dinfos->args[0], dinfos->maps_table);
    if (!procmap)
        return -1;

    long proc_addr = arg_to_long(procmap->line, 16);
    if (proc_addr == -1)
        return -1;

    intptr_t bp_addr   = -1;
    struct dw_file *dw = NULL;
    if (argsc == 3) {
        bp_addr = get_line_addr(dinfos->melf.dw_table, NULL, args[2], lineno);
    } else {
        dw = get_dw_file(dinfos, proc_addr);
        if (!dw)
            return -1;

        bp_addr = get_line_addr(dinfos->melf.dw_table, dw, NULL, lineno);
    }

    if (bp_addr == -1) {
        const char *file = (dw) ? dw->filename : args[2];
        fprintf(stderr, "Could not put a breakpoint at %s:%ld\n", file, lineno);
        return -1;
    }

    struct breakpoint *bp = bp_creat(BP_CLASSIC);
    return bp_set(dinfos, bp, (void *)(bp_addr + proc_addr), dinfos->dflt_pid);
}

shell_cmd(breakl, do_breakl, "Put a breakpoint on the line given in argument");
