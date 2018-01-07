#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "commands.h"
#include "inputs.h"
#include "maps.h"
#include "my_dbg.h"
#include "trace.h"
#include "dproc.h"

#define EXE_PATH_LEN 22 // strlen(/proc//exe) + int_max_len + '\0'

/**
** \brief Change the working directory to the directory where \p pid is.
**
** \note This operation is time and memory saving when debugging a file
** with debugging informations. It allows the debugger to not search and
** save the directories of source files which are in the same working
** directory than the debugged file.
*/
static void change_dir(pid_t pid)
{
    char dir_path[EXE_PATH_LEN] = { 0 };
    snprintf(dir_path, EXE_PATH_LEN, "/proc/%d/cwd", pid);

    if (chdir(dir_path) == -1)
        warn("Could not change the directory to %s, you still can debug but \
you may encounter problems while printing source code.", dir_path);
}

int do_attach(struct debug_infos *dinfos, char *args[])
{
    int argc = check_params(args, 2, 2);
    if (argc == -1)
        return -1;

    pid_t pid = arg_to_long(args[1], 10);
    if (pid == -1)
        return -1;

    fprintf(stderr, "attaching to %d ...\n", pid);

    char exe_path[EXE_PATH_LEN] = { 0 };
    snprintf(exe_path, EXE_PATH_LEN, "/proc/%d/exe", pid);

    size_t size;
    void *elf = map_elf(exe_path, &size);
    if (!elf)
        return -1;

    struct dproc *proc = dproc_creat();
    proc->pid = pid;
    if (attach_proc(proc) == -1)
        goto out_destroy_proc;

    if (dproc_htable_insert(proc, dinfos->dproc_table) == -1)
        goto out_destroy_proc;

    dinfos->dflt_pid = pid;
    dinfos->melf.elf  = elf;
    dinfos->melf.size = size;
    dinfos->args      = dup_args(args);
    get_symbols(&dinfos->melf);
    change_dir(pid);

    struct map *file_infos = parse_maps(dinfos->maps_table, pid);
    if (file_infos == NULL)
        goto out_empty_dinfos;

    free(dinfos->args[0]);
    // dup to avoid double free
    dinfos->args[0] = strdup(file_infos->line + file_infos->ofts[6]);
    fprintf(stderr, "done\n");

    return pid;

out_destroy_proc:
    dproc_destroy(proc);
    return -1;

out_empty_dinfos:
    empty_debug_infos(dinfos);
    return -1;
}

shell_cmd(attach, do_attach, "Attach to the pid given in argument");
