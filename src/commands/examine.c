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

// examine $format size start_addr [pid]

extern struct print_func print_functions[];

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
    if (*endptr || errno == ERANGE) {
        fprintf(stderr, "%s: Invalid argument\n", arg);
        return -1;
    }

    return res;
}

/*
** \brief parse the arguments given by the user and fill the corresponding
** parameters. If a pid is given, returns it.
**
** \return Return a potential pid or 0 if none.
*/
static int parse_args(char *args[], char *format,
                      size_t *size, uintptr_t *start_addr)
{
    size_t argsc = nullarray_size(args);
    if (argsc < 4 || argsc > 5) {
        fprintf(stderr, "Invalid number of argument\n");
        return -1;
    }

    if (!is_valid_format(args[1])) {
        fprintf(stderr, "%s: invalid argument\n", args[1]);
        return -1;
    }

    *format = args[1][1];
    *size   = arg_to_long(args[2], 10);
    if (*size == (size_t)-1)
        return -1;

    *start_addr = arg_to_long(args[3], 16);
    if (*start_addr == (uintptr_t)(-1))
        return -1;

    pid_t pid = 0;
    if (argsc == 5) {
        pid_t pid = arg_to_long(args[4], 10);
        if (pid == -1) {
            fprintf(stderr, "%s: invalid argument\n", args[4]);
            return -1;
        }
    }

    return pid;
}

/**
** \brief Read \p size bytes at \p start_addr in \proc
**
** \return Returns an allocated null-terminated string filled with the
** read bytes.
*/
static char *read_dproc(struct dproc *proc, size_t size, uintptr_t start_addr)
{
    int word     = sizeof(long);
    size_t alloc = size + word - (size % word) + 1;
    char *dumped = malloc(alloc);

    size_t nb_call = size / word + (size % word != 0);
    size_t i = 0;
    for ( ; i < nb_call; ++i) {
        void *addr   = (void *)(start_addr + i * word);
        long data = ptrace(PTRACE_PEEKTEXT, proc->pid, addr, NULL);
        if (data == -1)
            goto err_free_dumped;

        memcpy(dumped + (i * word), &data, word); 
    }

    memset(dumped + size, 0, size % word);
    return dumped;

err_free_dumped:
    free(dumped);
    warn("Failed to PEEKTEXT %d at %lx", proc->pid, start_addr + i * word);
    return NULL;
}

int do_examine(struct debug_infos *dinfos, char *args[])
{
    if (!dinfos->melf.elf || !dinfos->dflt_pid) {
        fprintf(stderr, "No running process\n");
        return -1;
    }

    char format;
    size_t size;
    uintptr_t start_addr;
    pid_t pid = parse_args(args, &format, &size, &start_addr);
    if (pid == -1)
        return -1;

    pid = (!pid) ? dinfos->dflt_pid : pid;
    struct dproc *proc = dproc_htable_get(pid, dinfos->dproc_table);
    if (!proc)
    {
        fprintf(stderr, "%d is not a valid pid\n", pid);
        return -1;
    }

    char *dumped = read_dproc(proc, size, start_addr);
    if (!dumped)
        return -1;

    get_print_func(format)(dumped, size, start_addr);
    free(dumped);

    return 0;
}

shell_cmd(examine, do_examine, "Examine memory at a given address");
