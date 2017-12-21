#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "commands.h"
#include "my_dbg.h"
#include "trace.h"
#include "dproc.h"
#include "print_func.h"

/**
** \param arr Null terminated array
**
** \return Return the size of \p arr
*/
static size_t nullarray_size(char *arr[])
{
    if (!arr)
        return 0;

    size_t size = 0;
    while (arr[size])
        ++size;

    return size;
}

static long arg_to_long(char *arg, int base)
{
    char *endptr = NULL;
    long res = strtol(arg, &endptr, base);
    if (*endptr || errno == ERANGE)
        return -1;

    return res;
}

static int check_params(char *args[])
{
    size_t size = nullarray_size(args);
    if (size > 2) {
        fprintf(stderr, "Too many arguments\n");
        return -1;
    }

    return size;
}

static struct dproc *get_dproc(struct debug_infos *dinfos, char *args[])
{
    pid_t pid = dinfos->dflt_pid;
    int argsc = check_params(args);
    if (argsc == -1)
        return NULL;
    else if (argsc == 2) {
        pid = arg_to_long(args[1], 10);
        if (pid == -1)
            goto err_invalid_pid;
    }

    struct dproc *proc = dproc_htable_get(pid, dinfos->dproc_table);
    if (!proc)
        goto err_invalid_pid;

    return proc;

err_invalid_pid:
    fprintf(stderr, "%d is not a valid pid\n", pid);
    return NULL;
}

int do_backtrace(struct debug_infos *dinfos, char *args[])
{
    if (!dinfos->melf.elf || !dinfos->dflt_pid) {
        fprintf(stderr, "No running process\n");
        return -1;
    }

    struct dproc *proc = get_dproc(dinfos, args);
    if (!proc)
        return -1;

    unw_cursor_t cp = proc->unw.c;
    unw_word_t ip;
    unw_word_t oft;
    while (unw_step(&cp) > 0) {
        if (unw_get_reg(&cp, UNW_REG_IP, &ip) < 0) {
            fprintf(stderr, "Could not get RIP register\n");
            return -1;
        }
        printf("0x%lx:", ip);

        char nam[4096];
        if (!unw_get_proc_name(&cp, nam, sizeof(nam), &oft)) {
            printf("\t%s+0x%lx\n", nam, oft);
        } else {
            printf("(\?\?\?\?\?\?)\n");
        }
    }

    return 0;

}

shell_cmd(backtrace, do_backtrace, "Print the call trace at the current %rip");
