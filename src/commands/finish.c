#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "commands.h"
#include "breakpoint.h"
#include "my_dbg.h"
#include "dproc.h"

int do_finish(struct debug_infos *dinfos, char *args[])
{
    int argsc = check_params(args, 1, 2);
    if (argsc == -1 || !is_traced(dinfos))
        return -1;

    struct dproc *proc = get_proc(dinfos, args, argsc, 1);
    if (!proc || !is_running(proc))
        return -1;

    unw_cursor_t cp = proc->unw.c;
    unw_word_t ip;
    if (unw_step(&cp) <= 0) {
        fprintf(stderr, "finish not meaningful in the outermost frame.\n");
        return -1;
    }

    if (unw_get_reg(&cp, UNW_REG_IP, &ip) < 0) {
        fprintf(stderr, "Could not get return address\n");
        return -1;
    }

    struct breakpoint *bp = bp_creat(BP_SILENT);
    bp_set(dinfos, bp, (void *)ip, dinfos->dflt_pid);

    // If bp_set fails it is because a breakpoint is already set at the finished
    // address. We do not need tu put a silent breakpoint.
    return do_continue(dinfos, NULL);
}

shell_cmd(finish, do_finish, "Continue the execution until hitting the return\
 address of the current function");
