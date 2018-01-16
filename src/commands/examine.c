#include <stdlib.h>
#include <stdio.h>

#include "commands.h"
#include "my_dbg.h"
#include "dproc.h"
#include "print_func.h"

// examine $format size [start_addr] [pid]

extern struct print_func print_functions[];

/*
** \brief parse the arguments given by the user and fill the corresponding
** parameters.
**
** \return Return the start_addr on success and -1 on failure.
*/
static long parse_args(pid_t pid, char *args[], int argsc, char *format,
                       size_t *size)
{
    if (!is_valid_format(args[1])) {
        fprintf(stderr, "%s: invalid argument\n", args[1]);
        return -1;
    }

    *format = args[1][1];
    *size   = arg_to_long(args[2], 10);
    if (*size == (size_t)-1)
        return -1;

    return get_addr(pid, args, argsc, 3);
}

int do_examine(struct debug_infos *dinfos, char *args[])
{
    if (!is_traced(dinfos))
        return -1;

    int argsc = check_params(args, 3, 5);
    if (argsc == -1)
        return -1;

    struct dproc *proc = get_proc(dinfos, args, argsc, 4);
    if (!proc || !is_running(proc))
        return -1;

    char format;
    size_t size;
    long start_addr = parse_args(proc->pid, args, argsc, &format, &size);
    if (start_addr == -1)
        return -1;

    char *dumped = read_dproc(dinfos, proc, size, start_addr);
    if (!dumped)
        return -1;

    get_print_func(format)(proc, dumped, size, start_addr);
    free(dumped);

    return 0;
}

shell_cmd(examine, do_examine, "Examine memory at a given address");
