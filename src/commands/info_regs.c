#include <stdio.h>

#include "commands.h"
#include "my_dbg.h"
#include "dproc.h"

struct reg {
    char *name;
    unw_regnum_t no;
};

static struct reg regs[] = {
    { "rip", UNW_X86_64_RIP },
    { "rsp", UNW_X86_64_RSP },
    { "rbp", UNW_X86_64_RBP },
    { "rax", UNW_X86_64_RAX },
    { "rdx", UNW_X86_64_RDX },
    { "rcx", UNW_X86_64_RCX },
    { "rbx", UNW_X86_64_RBX },
    { "rsi", UNW_X86_64_RSI },
    { "rdi", UNW_X86_64_RDI },
    { "r8", UNW_X86_64_R8 },
    { "r9", UNW_X86_64_R9 },
    { "r10", UNW_X86_64_R10 },
    { "r11", UNW_X86_64_R11 },
    { "r12", UNW_X86_64_R12 },
    { "r13", UNW_X86_64_R13 },
    { "r14", UNW_X86_64_R14 },
    { "r15", UNW_X86_64_R15 },
};

#define array_size(t) (sizeof(t) / sizeof(*t))

int print_regs(struct dproc *proc)
{
    if (unw_init_remote(&proc->unw.c, proc->unw.as,
                        proc->unw.ui) < 0) {
        fprintf(stderr, "Error while initing remote\n");
        return -1;
    }

    unw_word_t valp;
    size_t size = array_size(regs);
    for (size_t i = 0; i < size; ++i) {
        if (unw_get_reg(&proc->unw.c, regs[i].no, &valp) < 0) {
            fprintf(stderr, "Error while getting %s", regs[i].name);
            return -1;
        }

        printf("%s: 0x%lx\n", regs[i].name, valp);
    }

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
