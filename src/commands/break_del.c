#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "commands.h"
#include "my_dbg.h"
#include "breakpoint.h"

int do_break_del(struct debug_infos *dinfos, char *args[])
{
    if (!is_traced(dinfos))
        return -1;

    int argsc = check_params(args, 2, 2);
    if (argsc == -1)
        return -1;

    struct dproc *proc = get_proc(dinfos, args, argsc, 3);
    if (!proc)
        return -1;

    if (!is_running(proc))
        return -1;

    int is_hw = !strncmp(args[1], "dr", 2);

    long id = arg_to_long(args[1] + is_hw * 2, 10);
    if (id == -1)
        return -1;

    int ret = bp_htable_remove_by_id(dinfos, id, is_hw, dinfos->bp_table);
    if (ret == -1)
        fprintf(stderr, "Breakpoint %ld does not exist\n", id);

    return ret;
}

shell_cmd(break_del, do_break_del, "Remove the breakpoint given in argument");
