#include <elf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "commands.h"
#include "my_dbg.h"
#include "breakpoint.h"
#include "mapping.h"
#include "maps.h"

static inline void *add_oft(void *addr, uint64_t oft)
{
    char *ret = addr;
    return ret + oft;
}

static long got_addr_value(long proc_map_addr, const Elf64_Rela *rela,
                           pid_t pid)
{
    void *got_addr = (void *)(proc_map_addr + rela->r_offset);
    long got_value = ptrace(PTRACE_PEEKTEXT, pid, got_addr, NULL);
    if (got_value == -1)
        fprintf(stderr, "Could not peek value at %p\n", got_addr);

    return got_value;
}

int do_breakf(struct debug_infos *dinfos, char *args[])
{
    if (!is_running(dinfos))
       return -1;

    int argsc = check_params(args, 2, 2);
    if (argsc == -1)
        return -1;

    const Elf64_Sym *symbol = find_symbol(dinfos->melf, args[1]);
    if (!symbol) {
        printf("Could not found: %s\n", args[1]);
        return -1;
    }

    struct map *procmap = map_htable_get(dinfos->args[0], dinfos->maps_table);
    if (!procmap)
        return -1;

    long proc_map_addr = arg_to_long(procmap->line, 16);
    if (proc_map_addr == -1)
        return -1;

    uintptr_t bp_addr = proc_map_addr + symbol->st_value;
    if (symbol->st_value == 0)

    {
        const Elf64_Rela *rela = get_rela(dinfos->melf, symbol);
        long tst = got_addr_value(proc_map_addr, rela, dinfos->dflt_pid);
        if (tst == -1)
            return -1;

        bp_addr = tst;
    }

    struct breakpoint *bp = bp_creat(BP_CLASSIC);
    return bp_set(dinfos, bp, (void *)bp_addr, dinfos->dflt_pid);
}

shell_cmd(breakf, do_breakf, "Put a breakpoint on the function given in argument");
