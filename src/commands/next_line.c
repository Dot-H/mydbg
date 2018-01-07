#include <stdlib.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/reg.h>

#include "commands.h"
#include "breakpoint.h"
#include "maps.h"
#include "my_dbg.h"
#include "debug.h"

static long get_cur_addr(pid_t pid)
{
    long addr = ptrace(PTRACE_PEEKUSER, pid, sizeof(long)*RIP);
    if (addr == -1)
        fprintf(stderr, "Could not get the current address\n");

    return addr;
}

static inline int has_jumped(struct dw_file *dw, uintptr_t addr,
                             uintptr_t prv_addr, uintptr_t stp_addr)
{
    return addr < dw->start || addr >= dw->end // left file
           || addr < prv_addr // Jumped on previous line
           || addr > stp_addr; // Jumped line (return from function, goto..)
}

int do_next_line(struct debug_infos *dinfos, char *args[])
{
    if (!is_running(dinfos) || !has_debug_infos(dinfos))
        return -1;

    int argsc = check_params(args, 1, 1);
    if (argsc == -1)
        return -1;

    long addr = get_cur_addr(dinfos->dflt_pid);
    if (addr == -1)
        return -1;

    struct map *procmap = map_htable_get(dinfos->args[0], dinfos->maps_table);
    if (!procmap) {
        fprintf(stderr, "Could not find the mapping of %s\n", dinfos->args[0]);
        return -1;
    }

    long proc_addr = arg_to_long(procmap->line, 16);
    if (proc_addr == -1)
        return -1;

    addr -= proc_addr; /* We want to work on the elf offsets */
    struct dw_file *dw;
    intptr_t stp_addr = get_next_line_addr(dinfos->melf.dw_table, addr, &dw);
    if (stp_addr == -1) {
        fprintf(stderr, "Could not find the line corresponding to addr 0x%lx\n",
                addr);
        return -1;
    }

    uintptr_t prv_addr;
    while (stp_addr != addr) {
        prv_addr = addr;
        if (do_next_instr(dinfos, args) == -1)
            return -1;

        addr = get_cur_addr(dinfos->dflt_pid) - proc_addr;
        if (addr < 0)
            return -1;

        if (has_jumped(dw, addr, prv_addr, stp_addr))
            stp_addr = addr;
    }

    return 0;
}

shell_cmd(next_line, do_next_line, "Execute the code until the next source \
code line");
