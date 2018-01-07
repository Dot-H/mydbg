#include <err.h>
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>

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

/**
** \type Type of the breakpoint to put. Filled by the funciton.
**
** \return If the linker has already done the relocation, the value pointed
** to by the got is returned. Otherwise, it's the address which will be
** relocated which is returned. If an error occured, -1 is returned.
**
** \note An error is print on stderr if ptrace fails
*/
static long got_addr_value(struct map *procmap, long proc_map_addr,
                           const Elf64_Rela *rela, pid_t pid,
                           enum bp_type *type)
{
    void *got_addr = (void *)(proc_map_addr + rela->r_offset);
    long got_value = ptrace(PTRACE_PEEKTEXT, pid, got_addr, NULL);
    if (got_value == -1)
        warn("Could not peek value at %p\n", got_addr);

    printf("0x%lx\n", got_value);
    long proc_map_limit = arg_to_long(procmap->line + procmap->ofts[1], 16);
    if (proc_map_limit == -1)
        return -1;

    if (got_value > proc_map_addr && got_value < proc_map_limit) {
        *type = BP_RELOCATION;
        return (long)got_addr;
    }

    *type = BP_CLASSIC;
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
    if (!procmap) {
        fprintf(stderr, "Could not find the mapping of %s\n", dinfos->args[0]);
        return -1;
    }

    long proc_map_addr = arg_to_long(procmap->line, 16);
    if (proc_map_addr == -1)
        return -1;

    uintptr_t bp_addr = proc_map_addr + symbol->st_value;
    enum bp_type type = BP_CLASSIC;
    if (symbol->st_value == 0)

    {
        const Elf64_Rela *rela = get_rela(dinfos->melf, symbol);
        long tst = got_addr_value(procmap, proc_map_addr, rela,
                                  dinfos->dflt_pid, &type);
        if (tst == -1)
            return -1;

        bp_addr = tst;
    }

    struct breakpoint *bp = bp_creat(type);
    return bp_set(dinfos, bp, (void *)bp_addr, dinfos->dflt_pid);
}

shell_cmd(breakf, do_breakf, "Put a breakpoint on the function given in \
argument");
