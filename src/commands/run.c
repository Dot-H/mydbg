#include <stdio.h>
#include <sys/wait.h>

#include "commands.h"
#include "my_dbg.h"
#include "trace.h"
#include "dproc.h"

int do_run(struct debug_infos *dinfos, char *args[])
{
    (void)args;
    int status;
    int ret = -1;
    if (dinfos->melf.elf){
        ret = trace_binary(dinfos, &status);
        if (ret == -1)
            return -1;
    }
    else{
        fprintf(stderr, "No executable loaded\n");
        return -1;
    }

    if (WIFEXITED(status))
        return ret;

    struct dproc *new = dproc_creat(ret, status);
    dproc_htable_insert(new, dinfos->dproc_table);

    return ret;
}

shell_cmd(run, do_run, "Start debugged program");
