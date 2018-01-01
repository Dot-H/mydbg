#include <stdlib.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/reg.h>

#include "commands.h"
#include "breakpoint.h"
#include "maps.h"
#include "my_dbg.h"
#include "debug.h"

int do_next_line(struct debug_infos *dinfos, char *args[])
{
    if (!is_running(dinfos))
        return -1;

    int argsc = check_params(args, 1, 2);
    if (argsc == -1)
        return -1;

    long addr = ptrace(PTRACE_PEEKUSER, dinfos->dflt_pid, sizeof(long)*RIP);
    if (!addr)
        return -1;

    struct map *procmap = map_htable_get(dinfos->args[0], dinfos->maps_table);
    if (!procmap)
        return -1;

    long proc_addr = arg_to_long(procmap->line, 16);
    if (proc_addr == -1)
        return -1;

    intptr_t bp_addr = get_next_line_addr(dinfos->melf.dw_table,
                                          addr - proc_addr);
    if (bp_addr == -1) {
        fprintf(stderr, "Could not find the line corresponding to addr 0x%lx\n",
                addr);
        return -1;
    }

    bp_addr += proc_addr;
    struct breakpoint *bp = bp_creat(BP_SILENT);
    bp_set(dinfos, bp, (void *)bp_addr, dinfos->dflt_pid);

    // If bp_set fails it is because a breakpoint is already set at the finished
    // address. We do not need tu put a silent breakpoint.
    return do_continue(dinfos, NULL);
}

shell_cmd(next_line, do_next_line, "Execute the code until the next source code \
line");
