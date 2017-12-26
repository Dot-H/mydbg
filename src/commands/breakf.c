#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#include "commands.h"
#include "my_dbg.h"
#include "breakpoint.h"
#include "mapping.h"
#include "maps.h"

int do_breakf(struct debug_infos *dinfos, char *args[])
{
    if (!is_running(dinfos))
        return -1;

    int argsc = check_params(args, 2, 2);
    if (argsc == -1)
        return -1;

    const Elf64_Sym *symbol = find_symbol(dinfos->melf.elf, args[1]);
    if (!symbol)
        return -1;

    if (!symbol) {
        printf("Could not found: %s\n", args[1]);
    }

    struct map *procmap = map_htable_get(dinfos->args[0], dinfos->maps_table);
    if (!procmap)
        return -1;

    long dyn_addr = arg_to_long(procmap->line, 16);
    if (dyn_addr == -1)
        return -1;

    uintptr_t bp_addr = dyn_addr + symbol->st_value;
    struct breakpoint *bp = bp_creat(BP_CLASSIC);
    return bp_set(dinfos, bp, (void *)bp_addr, dinfos->dflt_pid);
}

shell_cmd(breakf, do_breakf, "Put a breakpoint on the function given in argument");
