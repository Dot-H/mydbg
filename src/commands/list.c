#include <stdlib.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/reg.h>

#include "commands.h"
#include "maps.h"
#include "my_dbg.h"
#include "debug.h"

/**
** \param noline line number searched
** \param start starting index
**
** \brief Search the index of \p noline into \p file starting at \p start
**
** \return Return the index of \p noline
*/
static size_t get_idx(const char *file, size_t noline, size_t start)
{
    size_t cnt = 1;
    for (; file[start] != EOF && cnt < noline; ++start)
        if (file[start] == '\n')
            ++cnt;

    return start;
}

/**
** \param dw file containing the source code to print
** \param line noline line number to print from
** \param nb number of line to print
**
** \brief print \p nb lines from \p dw starting at \p noline. If the \p dw's
** mfile attribute is unmap, map it.
**
** \return Return -1 in case of error and 0 otherwise.
**
** \note The function uses three static value to save the previous line and 
** its associated index. It is usefull to save time if the user do multiple
** call the list on the same file.
** In case of error while mapping an error is print on stderr.
*/
static int print_lines(struct dw_file *dw, size_t noline, size_t nb)
{
    static struct dw_file *last_dw = NULL;
    static size_t last_line = 0;
    static size_t last_idx  = 0;
    if (!dw->mfile)
        if (!dw_map(dw))
            return -1;

    if (last_dw != dw || noline < last_line) {
        last_dw   = dw;
        last_line = 0;
        last_idx  = 0;
    }

    size_t idx = get_idx(dw->mfile, noline - last_line, last_idx);
    if (dw->mfile[idx] == EOF) {
        fprintf(stderr, "Invalid line number: %zu", noline);
        return -1;
    }

    last_idx  = idx;
    last_line = noline; 

    for (size_t i = 0; i < nb && idx < dw->msize; ++i) {
        size_t end = get_idx(dw->mfile, 2, idx);
        printf(KBLU"%zu: "KNRM, noline + i); 
        idx += printf("%.*s", (int)(end - idx), dw->mfile + idx); 
    }

    return 0;
}

int do_list(struct debug_infos *dinfos, char *args[])
{
    if (!is_running(dinfos))
        return -1;

    ssize_t nb = 10;
    int argsc = check_params(args, 1, 2);
    if (argsc == -1)
        return -1;
    else if (argsc == 2) {
        nb = arg_to_long(args[1], 10);
        if (nb < 0)
            return -1;
    }

    long addr = ptrace(PTRACE_PEEKUSER, dinfos->dflt_pid, sizeof(long)*RIP);
    if (!addr)
        return -1;

    struct map *procmap = map_htable_get(dinfos->args[0], dinfos->maps_table);
    if (!procmap)
        return -1;

    long proc_addr = arg_to_long(procmap->line, 16);
    if (proc_addr == -1)
        return -1;

    struct dw_file *dw;
    ssize_t line = get_line_from_addr(dinfos->melf.dw_table, addr - proc_addr,
                                      &dw);
    if (line == -1) {
        fprintf(stderr, "Could not find the line corresponding to addr 0x%lx\n",
                addr);
        return -1;
    }

    return print_lines(dw, line, nb);
}

shell_cmd(list, do_list, "List N lines starting from the current one. N is by\
 default 10.");
