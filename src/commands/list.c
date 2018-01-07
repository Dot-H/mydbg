#include <stdlib.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/reg.h>

#include "commands.h"
#include "maps.h"
#include "my_dbg.h"
#include "debug.h"

/**
** \param lineno line number searched
** \param start starting index
**
** \brief Search the index of \p lineno into \p file starting at \p start
**
** \return Return the index of \p lineno
*/
static size_t get_idx(const char *file, size_t lineno, size_t start)
{
    size_t cnt = 1;
    for (; file[start] != EOF && cnt < lineno; ++start)
        if (file[start] == '\n')
            ++cnt;

    return start;
}

/**
** \param dw file containing the source code to print
** \param line lineno line number to print from
** \param nb number of line to print
**
** \brief print \p nb lines from \p dw starting at \p lineno. If the \p dw's
** mfile attribute is unmap, map it.
**
** \return Return -1 in case of error and 0 otherwise.
*/
static int print_lines(struct dw_file *dw, size_t lineno, size_t nb)
{
    if (!dw->mfile)
        if (!dw_map(dw))
            return -1;

    size_t idx = get_idx(dw->mfile, lineno, 0);
    if (dw->mfile[idx] == EOF) {
        fprintf(stderr, "Invalid line number: %zu", lineno);
        return -1;
    }

    for (size_t i = 0; i < nb && idx < dw->msize; ++i) {
        size_t end = get_idx(dw->mfile, 2, idx);
        printf(KBLU"%zu: "KNRM, lineno + i);
        idx += printf("%.*s", (int)(end - idx), dw->mfile + idx);
    }

    return 0;
}

int do_list(struct debug_infos *dinfos, char *args[])
{
    if (!is_running(dinfos) || !has_debug_infos(dinfos))
        return -1;

    ssize_t nb = 10;
    int argsc = check_params(args, 1, 3);
    if (argsc == -1)
        return -1;
    else if (argsc >= 2) {
        nb = arg_to_long(args[1], 10);
        if (nb < 0)
            return -1;
    }

    printf("Coucou args done\n");

    long addr = get_addr(dinfos->dflt_pid, args, argsc, 2);
    if (addr == -1)
        return -1;

    printf("Coucou addr 0x%lx\n", addr);
    struct map *procmap = map_htable_get(dinfos->args[0], dinfos->maps_table);
    if (!procmap) {
        fprintf(stderr, "Could not find the mapping of %s\n", dinfos->args[0]);
        return -1;
    }

    long proc_addr = arg_to_long(procmap->line, 16);
    if (proc_addr == -1)
        return -1;

    struct dw_file *dw;
    ssize_t line = get_line_from_addr(dinfos->melf.dw_table, addr - proc_addr,
                                      &dw);
    if (line == -1) {
        fprintf(stderr, "Cannot print the line corresponding to addr 0x%lx\n",
                addr - proc_addr);
        return -1;
    }

    printf("Coucou gonna print lines");
    return print_lines(dw, line, nb);
}

shell_cmd(list, do_list, "List N lines starting from the current one or the \
address given as second argument. N is by default 10");
