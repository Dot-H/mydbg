#include <err.h>
#include <stdio.h>
#include <string.h>
#include <sys/user.h>

#include "commands.h"
#include "my_dbg.h"
#include "dproc.h"

// macros algorithm from https://stackoverflow.com/questions/10452112/how-to-get-struct-member-with-a-string-using-macros-c/10452577

#define USER_FIELDS FIELD(r15) FIELD(r14) FIELD(r13) FIELD(r12) FIELD(rbp) \
    FIELD(rbx) FIELD(r11) FIELD(r10) FIELD(r9) FIELD(r8) FIELD(rax)        \
    FIELD(rcx) FIELD(rdx) FIELD(rsi) FIELD(rdi) FIELD(orig_rax) FIELD(rip) \
    FIELD(cs) FIELD(eflags) FIELD(rsp) FIELD(ss) FIELD(fs_base) FIELD(ds)  \
    FIELD(gs_base) FIELD(es) FIELD(fs) FIELD(gs)

static int set_fields(struct user_regs_struct *regs, const char *reg,
                        long val)
{
#   define FIELD(name)            \
    if (strcmp(#name,reg) == 0) { \
        regs->name = val;          \
        return 0;                 \
    }                            

    USER_FIELDS

#   undef FIELD

    return -1;
}

int set_reg(struct dproc *proc, const char *reg, long val)
{
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, proc->pid, 0, &regs) == -1) {
        warn("Could not get registers");
        return -1;
    }

    if (set_fields(&regs, reg, val) == -1)
        return -1;

    if (ptrace(PTRACE_SETREGS, proc->pid, 0, &regs) == -1) {
        warn("Could not set registers");
        return -1;
    }

    return 0;
}

int do_set_reg(struct debug_infos *dinfos, char *args[])
{
    if (!is_traced(dinfos))
        return -1;

    int argsc = check_params(args, 3, 4);
    if (argsc == -1)
        return -1;

    struct dproc *proc = get_proc(dinfos, args, argsc, 3);
    if (!proc)
        return -1;

    long val = arg_to_long(args[2], 16);
    if (val == -1)
        return -1;

    int ret = set_reg(proc, args[1], val);
    if (ret == -1)
        fprintf(stderr, "Unknown register %s\n", args[1]);

    return ret;
}

shell_cmd(set, do_set_reg, "Set a register to the given value");
