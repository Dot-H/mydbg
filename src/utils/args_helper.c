#include <stdlib.h>
#include <err.h>
#include "errno.h"
#include <stdio.h>

#include "args_helper.h"

size_t nullarray_size(char *arr[])
{
    if (!arr)
        return 0;

    size_t size = 0;
    while (arr[size])
        ++size;

    return size;
}

long arg_to_long(char *arg, int base)
{
    char *endptr = NULL;
    long res = strtol(arg, &endptr, base);
    if (*endptr || errno == ERANGE) {
        fprintf(stderr, "%s: Invalid argument\n", arg);
        return -1;
    }

    return res;
}

int check_params(char *args[], long min, long max)
{
    if (!args && min == 1)
        return 1;

    long size = nullarray_size(args);
    if (max != -1 && size > max) {
        fprintf(stderr, "Too many arguments\n");
        return -1;
    } else if (size < min) {
        fprintf(stderr, "Not enough argument\n");
        return -1;
    }

    return size;
}

int is_running(struct debug_infos *dinfos)
{
    if (!dinfos->melf.elf || !dinfos->dflt_pid) {
        fprintf(stderr, "No running process\n");
        return 0;
    }

    return 1;
}

int has_debug_infos(struct debug_infos *dinfos)
{
    if (!dinfos->melf.dw_table)
    {
        fprintf(stderr, "No debugging infos found\n");
        return 0;
    }

    return 1;
}

struct dproc *get_proc(struct debug_infos *dinfos, char *args[], int argsc,
                       int idx)
{
    pid_t pid = dinfos->dflt_pid;
    if (argsc > idx) {
        pid = arg_to_long(args[idx], 10);
        if (pid == -1)
            return NULL;
    }

    struct dproc *proc = dproc_htable_get(pid, dinfos->dproc_table);
    if (!proc)
        fprintf(stderr, "Could not find process %d\n", pid);

    return proc;
}
