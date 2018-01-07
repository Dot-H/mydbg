#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "commands.h"
#include "my_dbg.h"
#include "breakpoint.h"

static long mask_value(enum bp_hw_len len, long value)
{
    switch (len) {
        case BP_HW_LEN_1BY:
            return value & 0xFF;
        case BP_HW_LEN_2BY:
            return value & 0xFFFF;
        case BP_HW_LEN_4BY:
            return value & 0xFFFFFFFF;
        case BP_HW_LEN_8BY:
            return value & 0xFFFFFFFFFFFFFFFF;
    }

    return -1; // Should not happen
}

/**
** \brief parse the condition and the len given as respectively second and
** third argument and filled params \p cond and \p len with the values. If no
** parameter is given for an argument, the argument is set to its default value
**
** \return Return the number of argument parse. 0 if none, 1 if cond has been
** given by user and 2 if len has been given too. If an error occurs, -1 is
** returned.
**
** \note The default value of cond is BP_HW_WRONLY and the default value of
** len is BP_HW_LEN_8BY.
**
** \note In case of error, a message is print on stderr.
*/
static int parse_args(char *args[], int argsc, enum bp_hw_cond *cond,
                      enum bp_hw_len *len)
{
    *cond = BP_HW_WRONLY;
    *len  = BP_HW_LEN_8BY;
    if (argsc < 3)
        return 0;

    if (!strcmp(args[2], "rw"))
        *cond = BP_HW_RDWR_NOFETCH;
    else if (!strcmp(args[2], "w"))
        *cond = BP_HW_WRONLY;
    else {
        fprintf(stderr, "Invalid argument: %s\n", args[2]);
        return -1;
    }

    if (argsc == 3)
        return 1;

    if (args[3][1] != '\0') {
        fprintf(stderr, "invalid argument: %s\n", args[3]);
        return -1;
    }

    char len_input = args[3][0];
    if (len_input == '1')
        *len = BP_HW_LEN_1BY;
    else if (len_input == '2')
        *len = BP_HW_LEN_2BY;
    else if (len_input == '4')
        *len = BP_HW_LEN_4BY;
    else if (len_input == '8')
        *len = BP_HW_LEN_8BY;
    else {
        fprintf(stderr, "Invalid argument: %s\n", args[3]);
        return -1;
    }

    return 2;
}

int do_wp(struct debug_infos *dinfos, char *args[])
{
    if (!is_running(dinfos))
        return -1;

    int argsc = check_params(args, 2, 5);
    if (argsc == -1)
        return -1;

    int pid = get_pid(dinfos, args, argsc, 4);
    if (pid == -1)
        return -1;

    long bp_addr = get_addr(pid, args, argsc, 1);
    if (bp_addr == -1)
        return -1;

    enum bp_hw_cond cond;
    enum bp_hw_len len;
    if (parse_args(args, argsc, &cond, &len) == -1)
        return -1;


    struct breakpoint *bp = bp_creat(BP_HARDWARE);
    bp->a_pid = pid;
    bp->addr  = (void *)bp_addr;
    bp->state = BP_ENABLED;

    if (bp_hw_poke(dinfos, bp, cond, len) < 0)
        goto err_destroy_bp;

    if (bp_htable_insert(bp, dinfos->bp_table) == -1)
        goto err_destroy_bp;

    bp->sv_instr = ptrace(PTRACE_PEEKTEXT, pid, (void *)bp_addr, 0);
    if (bp->sv_instr == -1 && errno != 0) {
        fprintf(stderr, "Failed to get current value at 0x%lx. Saved value is \
set to 0\n", bp_addr);
        bp->sv_instr = 0;
    }

    bp->sv_instr = mask_value(len, bp->sv_instr);
    return 0;

err_destroy_bp:
    bp_destroy(dinfos, bp);
    return -1;
}

shell_cmd(watchpoint, do_wp, "Put a hardwarre watchpoint on the address, \
the rights and the size given in argument");
