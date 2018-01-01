#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <capstone/capstone.h>

#include "commands.h"
#include "breakpoint.h"
#include "my_dbg.h"
#include "dproc.h"
#include "trace.h"

#define TST_LEN 16

static void *get_stopped_addr(struct dproc *proc)
{
    if (unw_init_remote(&proc->unw.c, proc->unw.as,
                        proc->unw.ui) < 0) {
        fprintf(stderr, "Error while initing remote\n");
        return NULL;
    }

    unw_word_t rip;
    if (unw_get_reg(&proc->unw.c, UNW_X86_64_RIP, &rip) < 0) {
            fprintf(stderr, "Error while getting RIP");
            return NULL;
    }

    return (void *)(rip);
}

static void *check_call(char *str, uintptr_t addr)
{
    csh handle;
    cs_insn *insn;
    void *ret = NULL;

    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) {
        fprintf(stderr, "Failed to initialize capstone\n");
        return ret;
    }

    size_t count = cs_disasm(handle, (uint8_t *)str, TST_LEN, addr, 0, &insn);
    if (count > 0) {
        if (!strcmp(insn[0].mnemonic, "call"))
            ret = (void *)insn[1].address;

        cs_free(insn, count);
    }
    else
        fprintf(stderr, "Failed to disassemble given code\n");

    cs_close(&handle);
    return ret;
}

static int jump_call(struct debug_infos *dinfos, struct dproc *proc,
                     void *nxt_addr)
{
    struct breakpoint *bp = bp_creat(BP_SILENT);
    bp->a_pid              = proc->pid;

    bp->sv_instr = set_opcode(bp->a_pid, BP_OPCODE, nxt_addr);
    if (bp->sv_instr == -1)
        goto err_free_bp;

    bp->addr  = (void *)nxt_addr;
    bp->state = BP_ENABLED;

    if (bp_htable_insert(bp, dinfos->bp_table) == -1)
    {
        fprintf(stderr, "A breakpoint is already set at %p\n",  bp->addr);
        goto err_free_bp;
    }

    return do_continue(dinfos, NULL);

err_free_bp:
    free(bp);
    return -1;
}

int do_next_instr(struct debug_infos *dinfos, char *args[])
{
    if (!is_running(dinfos))
        return -1;

    int argsc = check_params(args, 1, 2);
    if (!argsc)
        return -1;

    struct dproc *proc = get_proc(dinfos, args, argsc, 1);
    if (!proc)
        return -1;

    if (bp_cont(dinfos, proc) == -1)
        return -1;

    uintptr_t bp_addr = (uintptr_t)get_stopped_addr(proc);
    char *dumped = read_dproc(dinfos, proc, 8, bp_addr);
    if (!dumped)
        return -1;

    int ret = 0;
    void *nxt_addr = check_call(dumped, bp_addr);
    if (!nxt_addr) {
        if (ptrace(PTRACE_SINGLESTEP, proc->pid, 0, 0) == -1) {
            warn("Could not singlstep in %d", proc->pid);
            goto err_free_dumped;
        }

        wait_tracee(dinfos, proc);
    } else {
        ret = jump_call(dinfos, proc, nxt_addr);
    }

    free(dumped);

    return ret;

err_free_dumped:
    free(dumped);
    return -1;
}

shell_cmd(next_instr, do_next_instr, "Step one instruction, but proceed through\
 subroutine calls");
