#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "commands.h"
#include "args_helper.h"
#include "my_dbg.h"
#include "trace.h"
#include "dproc.h"
#include "print_func.h"

static struct dproc *get_dproc(struct debug_infos *dinfos, char *args[])
{
    pid_t pid = dinfos->dflt_pid;
    int argsc = check_params(args, 1, 2);
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
    if (check_params(args, 1, 1) == -1 || !is_running(dinfos))
        return -1;

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
            printf("\t%s <+%ld>\n", nam, oft);
        } else {
            printf("(\?\?\?\?\?\?)\n");
        }
    }

    return 0;

}

shell_cmd(backtrace, do_backtrace, "Print the call trace at the current %rip");
