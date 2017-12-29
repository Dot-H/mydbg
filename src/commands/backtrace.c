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

int do_backtrace(struct debug_infos *dinfos, char *args[])
{
    int argsc = check_params(args, 1, 2);
    if (argsc == -1 || !is_running(dinfos))
        return -1;

    struct dproc *proc = get_proc(dinfos, args, argsc, 1);
    if (!proc)
        return -1;

    unw_cursor_t cp = proc->unw.c;
    unw_word_t ip = 0;
    unw_word_t oft;
    while (unw_step(&cp) > 0) {
        if (unw_get_reg(&cp, UNW_REG_IP, &ip) < 0) {
            fprintf(stderr, "Could not get return address\n");
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

    if (ip == 0) {
        fprintf(stderr, "backtrace not meaningful in the outermost frame.\n");
        return -1;
    }

    return 0;

}

shell_cmd(backtrace, do_backtrace, "Print the call trace at the current %rip");
