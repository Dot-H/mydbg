#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/reg.h>

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

int is_traced(struct debug_infos *dinfos)
{
    if (!dinfos->melf.elf || !dinfos->dflt_pid) {
        fprintf(stderr, "No traced process\n");
        return 0;
    }

    return 1;
}

int is_running(struct dproc *proc)
{
    if (is_finished(proc)) {
        fprintf(stderr, "%d is not running anymore\n", proc->pid);
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

long get_addr(pid_t pid, char *args[], int argsc, int idx)
{
    long addr = -1;
    if (argsc <= idx) {
        addr = ptrace(PTRACE_PEEKUSER, pid, sizeof(long) * RIP);
        if (addr == -1)
            warn("Could not get the current address");

    } else {
        addr = arg_to_long(args[idx], 16);
    }

    return addr;
}

pid_t get_pid(struct debug_infos *dinfos, char *args[], int argsc, int idx)
{
    pid_t pid = dinfos->dflt_pid;
    if (argsc <= idx)
        return pid;

    pid = arg_to_long(args[idx], 10);
    if (pid < -1) {
        fprintf(stderr, "Pid cannot be negative: %d\n", pid);
        return -1;
    }

    return pid;
}
