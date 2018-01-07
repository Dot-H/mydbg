#include <err.h>
#include <stdio.h>
#include <sys/user.h>

#include "commands.h"
#include "my_dbg.h"
#include "dproc.h"

#define array_size(t) (sizeof(t) / sizeof(*t))

#define print_reg(regs, reg)                                   \
    printf("%s: 0x%llx\n", #reg, regs.reg)

int print_regs(struct dproc *proc)
{
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, proc->pid, 0, &regs) == -1) {
        warn("Could not get registers");
        return -1;
    }

    print_reg(regs, rip);
    print_reg(regs, rsp);
    print_reg(regs, rbp);
    print_reg(regs, eflags);
    print_reg(regs, orig_rax);
    print_reg(regs, rax);
    print_reg(regs, rbx);
    print_reg(regs, rcx);
    print_reg(regs, rdx);
    print_reg(regs, rdi);
    print_reg(regs, rsi);
    print_reg(regs, r8);
    print_reg(regs, r9);
    print_reg(regs, r10);
    print_reg(regs, r11);
    print_reg(regs, r12);
    print_reg(regs, r13);
    print_reg(regs, r14);
    print_reg(regs, r15);
    print_reg(regs, cs);
    print_reg(regs, ds);
    print_reg(regs, es);
    print_reg(regs, fs);
    print_reg(regs, gs);
    print_reg(regs, ss);
    print_reg(regs, fs_base);
    print_reg(regs, gs_base);

    return 0;
}

int do_info_regs(struct debug_infos *dinfos, char *args[])
{
    if (!is_traced(dinfos))
        return -1;

    int argsc = check_params(args, 1, 2);
    if (argsc == -1)
        return -1;

    struct dproc *proc = get_proc(dinfos, args, argsc, 1);
    if (!proc)
        return -1;

    return print_regs(proc);
}

shell_cmd(info_regs, do_info_regs, "Print the studied process's registers");
