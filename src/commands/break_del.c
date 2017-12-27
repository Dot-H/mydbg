#include <stdlib.h>
#include <stdio.h>

#include "commands.h"
#include "my_dbg.h"
#include "breakpoint.h"

int do_break_del(struct debug_infos *dinfos, char *args[])
{
    if (!is_running(dinfos))
        return -1;

    int argsc = check_params(args, 2, 2);
    if (argsc == -1)
        return -1;

    long id = arg_to_long(args[1], 10);
    if (id == -1)
        return -1;

    int ret = bp_htable_remove_by_id(id, dinfos->bp_table);
    if (ret == -1)
        fprintf(stderr, "Breakpoint %ld does not exist\n", id);

    return ret;
}

shell_cmd(break_del, do_break_del, "Remove the breakpoint given in argument");
