#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "commands.h"
#include "inputs.h"
#include "maps.h"
#include "my_dbg.h"
#include "trace.h"
#include "dproc.h"

static void copy_args(struct debug_infos *dinfos, char *args[])
{
    char **copy = dup_args(args);
    free(copy[0]);
    copy[0] = strdup(dinfos->args[0]);
    destroy_args(dinfos->args);

    dinfos->args = copy;
}


int do_run(struct debug_infos *dinfos, char *args[])
{
    int argc = check_params(args, 1, -1);
    if (argc == -1)
        return -1;
    else if (argc >= 2)
        copy_args(dinfos, args);

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
    if (dproc_htable_insert(proc, dinfos->dproc_table) == -1)
        goto out_destroy_proc;

    dinfos->dflt_pid = pid;
    if (parse_maps(dinfos->maps_table, pid) == NULL)
        goto out_destroy_proc;

    return pid;

out_destroy_proc:
    dproc_destroy(proc);
    return -1;
}

shell_cmd(run, do_run, "Start debugged program");
