#include <stdio.h>
#include <err.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/reg.h>

#include "breakpoint.h"
#include "commands.h"
#include "trace.h"

#define BP_ERR (void *)-1

static int hit_classic(struct debug_infos *dinfos, struct breakpoint *bp,
                       struct dproc *proc);
static int hit_temporary(struct debug_infos *dinfos, struct breakpoint *bp,
                         struct dproc *proc);
static int hit_hardware(struct debug_infos *dinfos, struct breakpoint *bp,
                        struct dproc *proc);

/* Handlers' index is given by the value of their type */
int (*bp_handlers[])(struct debug_infos *dinfos, struct breakpoint *bp,
                     struct dproc *proc) = {
    hit_classic,
    hit_temporary,
    hit_temporary, // BP_SILENT handling
    hit_hardware
};

static struct breakpoint *bp_out_sys = NULL;

/*
** \return Return -1 on failure and %rax on success.
**
** \note An error is print on stderr in case of failure.
*/
static long get_sysno(struct dproc *proc)
{
    long rax = ptrace(PTRACE_PEEKUSER, proc->pid, sizeof(long)*ORIG_RAX);
    if (rax == -1)
        fprintf(stderr, "%d: Failed to get syscall number\n", proc->pid);

    return rax;
}

/*
** \brief get the address of the breakpoint's instruction hit by \p proc
** which is %rip - 1.
**
** \return Return a pointer to the address on success and the macro BP_ERR
** on failure.
**
** \note An error is print on stderr in case of failure.
*/
static void *get_stopped_addr(struct dproc *proc)
{
    if (unw_init_remote(&proc->unw.c, proc->unw.as,
                        proc->unw.ui) < 0) {
        fprintf(stderr, "Error while initing remote\n");
        return BP_ERR;
    }

    unw_word_t rip;
    if (unw_get_reg(&proc->unw.c, UNW_X86_64_RIP, &rip) < 0) {
            fprintf(stderr, "Error while getting RIP");
            return BP_ERR;
    }

    return (void *)(rip - 1);
}

/*
** \brief Set rip to the breakpoint's address hit by \p proc and replace
** the BP_OPCODE by the breakpoint's saved instruction. Update the breakpoint's
** count and state.
**
** \return Return -1 on failure and 0 on success.
**
** \note An error is print on stderr in case of failure.
*/
static int hit_classic(struct debug_infos *dinfos, struct breakpoint *bp,
                       struct dproc *proc)
{
    (void)dinfos;
    printf("Hit breakpoint %u at %p\n", bp->id, bp->addr);

    if (unw_set_reg(&proc->unw.c, UNW_X86_64_RIP, (uintptr_t)bp->addr) < 0) {
        fprintf(stderr, "Could not set RIP to %p\n", bp->addr);
        return -1;
    }

    if (set_opcode(proc->pid, bp->sv_instr, bp->addr) == -1)
        return -1;

    bp->state = BP_HIT;
    bp->count += 1;

    return 0;
}

/*
** \brief Set rip to the breakpoint's address hit by \p proc and replace
** the BP_OPCODE by the breakpoint's saved instruction. Once it is done
** the breakpoint is removed from the bp_table and destroyed.
**
** \return Return -1 on failure and 0 on success.
**
** \note An error is print on stderr in case of failure.
*/
static int hit_temporary(struct debug_infos *dinfos, struct breakpoint *bp,
                         struct dproc *proc)
{
    if (bp->type != BP_SILENT)
        printf("Hit temporary breakpoint %u at %p\n", bp->id, bp->addr);

    if (set_opcode(proc->pid, bp->sv_instr, bp->addr) == -1) {
        fprintf(stderr, "Could not restore instruction at %p\n", bp->addr);
        goto err_reset_bp;
    }

    if (unw_set_reg(&proc->unw.c, UNW_X86_64_RIP, (uintptr_t)bp->addr) < 0) {
        fprintf(stderr, "Could not set RIP to %p\n", bp->addr);
        goto err_reset_bp;
    }

    bp_htable_remove(bp, dinfos->bp_table);

    return 0;

err_reset_bp:
    set_opcode(proc->pid, BP_OPCODE, bp->addr);
    return -1;
}

static int hit_hardware(struct debug_infos *dinfos, struct breakpoint *bp,
                        struct dproc *proc)
{
    (void)dinfos;
    (void)proc;
    printf("Hit hardware breakpoint %u at 0x%lx\n", bp->id,
                                                (uintptr_t)bp->addr + 1);
    return 0;
}

/**
** \brief Print the return value of the syscall and set bp_out_sys to
** null.
**
** \return Return -1 on failure and 0 on success.
**
** \note In case of failure bp_out_sys is still set to NULL and an error
** is print on stderr.
*/
static int out_syscall(struct breakpoint *bp, struct dproc *proc)
{
    bp_out_sys = NULL;
    long rax = ptrace(PTRACE_PEEKUSER, proc->pid, sizeof(long)*RAX);
    if (rax == -1) {
        fprintf(stderr, "%d: Failed to get syscall number\n", proc->pid);
        return -1;
    }

    printf("Returned from syscall %p with value %zu\n", bp->addr, rax);
    return 0;
}

int bp_sys_hit(struct debug_infos *dinfos, struct dproc *proc)
{
    long sysno = get_sysno(proc);
    if (sysno == -1)
        return -1;

    struct breakpoint *bp = bp_htable_get((void *)sysno, dinfos->bp_table);
    if (!bp)
        return -1;

    if (bp_out_sys == bp)
        return out_syscall(bp, proc);

    printf("Hit breakpoint %u on syscall %zu\n", bp->id, (size_t)bp->addr);

    bp->count += 1;
    bp_out_sys = bp;

    return 0;
}

int bp_hit(struct debug_infos *dinfos, struct dproc *proc)
{
    void *bp_addr = get_stopped_addr(proc);
    if (bp_addr == BP_ERR)
        return -1;

    struct breakpoint *bp = bp_htable_get(bp_addr, dinfos->bp_table);
    if (!bp)
        return -1;

    if (bp->state == BP_HIT || bp->state == BP_DISABLED)
        return -1;

    return bp_handlers[bp->type](dinfos, bp, proc);
}
