#include <stdio.h>

#include "commands.h"
#include "my_dbg.h"
#include "trace.h"

int do_run(struct debug_infos *dinfos, const char *args[])
{
    (void)args;
    int ret = 1;
    if (dinfos->melf.elf)
        ret = trace_binary(dinfos);
    else
        fprintf(stderr, "No executable loaded\n");

    return ret;
}

shell_cmd(run, do_run, "Start debugged program");
