#include <stdio.h>
#include <signal.h>

#include "breakpoint.h"
#include "commands.h"
#include "trace.h"

static int hit_classic(struct debug_infos *dinfos, struct breakpoint *bp,
                       struct dproc *proc);
static int hit_temporary(struct debug_infos *dinfos, struct breakpoint *bp,
                         struct dproc *proc);
static int hit_reset(struct debug_infos *dinfos, struct breakpoint *bp,
                     struct dproc *proc);

int (*bp_handlers[])(struct debug_infos *dinfos, struct breakpoint *bp,
                     struct dproc *proc) = {
    hit_classic,
    hit_temporary,
    hit_reset
};

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

    return (void *)(rip - 1);
}

static int hit_reset(struct debug_infos *dinfos, struct breakpoint *bp,
                     struct dproc *proc)
{
    if (set_opcode(proc->pid, bp->sv_instr, bp->addr) == -1)
        goto err_reset_bp;

    void *bp_orig_addr = (void *)((uintptr_t)bp->addr - 1);
    struct breakpoint *bp_orig = bp_htable_get(bp_orig_addr, dinfos->bp_table);
    if (!bp_orig) {
        fprintf(stderr, "Could not find origin bp at %p\n", bp_orig_addr);
        goto err_reset_bp;
    }

    if (set_opcode(proc->pid, BP_OPCODE, bp_orig->addr) == -1)
        goto err_reset_bp;

    if (unw_set_reg(&proc->unw.c, UNW_X86_64_RIP, (uintptr_t)bp->addr) < 0) {
        fprintf(stderr, "Could not set RIP to %p", bp->addr);
        goto err_reset_bp;
    }

    bp_htable_remove(bp, dinfos->bp_table);
    bp_orig->is_enabled = 1;

    do_continue(dinfos, NULL);
    wait_tracee(dinfos, proc);

    return 0;

err_reset_bp:
    set_opcode(proc->pid, BP_OPCODE, bp->addr);
    return -1;
}

static int hit_classic(struct debug_infos *dinfos, struct breakpoint *bp,
                       struct dproc *proc)
{
    printf("Hit breakpoint %u at %p\n", bp->id, bp->addr);
    if (set_opcode(proc->pid, bp->sv_instr, bp->addr) == -1)
        goto err_reset_bp;

    if (unw_set_reg(&proc->unw.c, UNW_X86_64_RIP, (uintptr_t)bp->addr) < 0) {
        fprintf(stderr, "Could not set RIP to %p", bp->addr);
        goto err_reset_bp;
    }

    bp->is_enabled = 0; /* Will be reabled by reset bp */
    if (bp_create_reset(dinfos->bp_table, bp) == -1)
        goto err_reset_bp;

    bp->count += 1;
    return 0;

err_reset_bp:
    set_opcode(proc->pid, BP_OPCODE, bp->addr);
    return -1;
}

static int hit_temporary(struct debug_infos *dinfos, struct breakpoint *bp,
                         struct dproc *proc)
{
    printf("Hit temporary breakpoint %u at %p\n", bp->id, bp->addr);
    if (set_opcode(proc->pid, bp->sv_instr, bp->addr) == -1) {
        fprintf(stderr, "Could not restore instruction at %p", bp->addr);
        goto err_reset_bp;
    }

    if (unw_set_reg(&proc->unw.c, UNW_X86_64_RIP, (uintptr_t)bp->addr) < 0) {
        fprintf(stderr, "Could not set RIP to %p", bp->addr);
        goto err_reset_bp;
    }

    bp_htable_remove(bp, dinfos->bp_table);

    return 0;

err_reset_bp:
    set_opcode(proc->pid, BP_OPCODE, bp->addr);
    return -1;
}

int bp_hit(struct debug_infos *dinfos, struct dproc *proc)
{
    siginfo_t sig = proc->siginfo;
    if (sig.si_signo != SIGTRAP) // || sig.si_code != TRAP_BRKPT)
        return -1;

    void *bp_addr = get_stopped_addr(proc);
    if (!bp_addr)
        return -1;

    struct breakpoint *bp = bp_htable_get(bp_addr, dinfos->bp_table);
    if (!bp)
        return -1;

    if (!bp->is_enabled)
        return -1;

    return bp_handlers[bp->type](dinfos, bp, proc);

    return 0;
}
