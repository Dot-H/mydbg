#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "commands.h"
#include "maps.h"
#include "my_dbg.h"
#include "trace.h"
#include "dproc.h"

int do_run(struct debug_infos *dinfos, char *args[])
{
    (void)args;
    struct dproc *proc = dproc_creat();

    pid_t pid = -1;
    if (dinfos->melf.elf){
        pid = trace_binary(dinfos, proc);
        if (pid == -1)
            goto out_destroy_proc;
    }
    else{
        fprintf(stderr, "No executable loaded\n");
        goto out_destroy_proc;
    }

    if (WIFEXITED(proc->status))
        goto out_destroy_proc;

    proc->pid = pid;
    dproc_htable_insert(proc, dinfos->dproc_table);
    dinfos->dflt_pid = pid;
    dinfos->maps_table = parse_maps(pid);
    if (!dinfos->maps_table)
        goto out_destroy_proc;

    return pid;

out_destroy_proc:
    dproc_destroy(proc);
    return pid;
}

shell_cmd(run, do_run, "Start debugged program");
