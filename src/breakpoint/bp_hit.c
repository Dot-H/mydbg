#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/user.h>

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
    hit_hardware,
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

    bp_htable_remove(dinfos, bp, dinfos->bp_table);

    return 0;

err_reset_bp:
    set_opcode(proc->pid, BP_OPCODE, bp->addr);
    return -1;
}

/**
** \brief Get %dr7 and %dr6 values and fills \p dr7 and \p dr6 with it.
**
** \return Return the dr offset on success and -1 in case of error.
**
** \note If an error occured, a message is print on stderr.
*/
static int get_dr(struct dproc *proc, struct dr7 **dr7, struct dr6 **dr6)
{
    long dr6_tst = ptrace(PTRACE_PEEKUSER, proc->pid, DR_OFFSET(6), 0);
    if (dr6_tst == -1) {
        warn("Could not get %%dr6 in process %d", proc->pid);
        return -1;
    }

    long dr7_tst = ptrace(PTRACE_PEEKUSER, proc->pid, DR_OFFSET(7), 0);
    if (dr7_tst == -1) {
        warn("Could not get %%dr7 in process %d", proc->pid);
        return -1;
    }

    *dr7 = (struct dr7 *)(&dr7_tst);
    *dr6 = (struct dr6 *)(&dr6_tst);
    unsigned i = 0;
    for (; i < BP_NHW; ++i) {
        unsigned dtcd;
        dr_get_hwlabel(*dr6, detected, i, dtcd)
        if (!dtcd)
            continue;

        unsigned ena;
        dr_get_hwlabel(*dr7, local, i, ena);
        if (ena)
            break;
    }

    if (i == BP_NHW) {
        fprintf(stderr, "Something went wrong in hit_hardware..");
        return -1;
    }

    return i;
}

static long mask_value(struct dr7 *dr7, int dr_offset, long value)
{
    enum bp_hw_len dr7_len;
    dr_get_hwlabel(dr7, len, dr_offset, dr7_len)
    switch (dr7_len) {
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

static int hit_hardware(struct debug_infos *dinfos, struct breakpoint *bp,
                        struct dproc *proc)
{
    (void)dinfos;
    (void)proc;
    printf("Hit hardware breakpoint dr%u at 0x%lx\n", bp->id,
                                            (uintptr_t)bp->addr + 1);

    return 0;
}

static int hit_watchpoint(struct debug_infos *dinfos, struct dproc *proc)
{
    struct dr7 *dr7 = NULL;
    struct dr6 *dr6 = NULL;
    int oft = -1;
    if ((oft = get_dr(proc, &dr7, &dr6)) == -1)
        return -1;

    struct breakpoint *bp = bp_htable_get((void *)(proc->bp_hwtab[oft] - 1),
                                          dinfos->bp_table);
    if (!bp)
        return -1;

    printf("Hit hardware watchpoint dr%u at 0x%lx\n", bp->id,
                                            (uintptr_t)bp->addr + 1);

    enum bp_hw_cond dr7_cond = 0;
    dr_get_hwlabel(dr7, cond, oft, dr7_cond)
    if (dr7_cond == BP_HW_INSTR)
        return 0;

    long new_value = ptrace(PTRACE_PEEKTEXT, bp->a_pid, proc->bp_hwtab[oft], 0);
    if (new_value == -1) {
        warn("Could not get the new value at 0x%lx in %d\n",
             proc->bp_hwtab[oft], bp->a_pid);
        bp->sv_instr = 0;
        return -1;
    }

    new_value = mask_value(dr7, oft, new_value);
    if (new_value == bp->sv_instr) {
        printf("Read access\n");
    } else {
        printf("old value: 0x%ld\nnew value: 0x%ld\n", bp->sv_instr, new_value);
        bp->sv_instr = new_value;
    }

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

/**
** \return Return 0 if no hardware breakpoint are put and 1 otherwise.
*/
static int has_hwbp(struct dproc *proc)
{
    for (int i = 0; i < BP_NHW; ++i)
        if (proc->bp_hwtab[i])
            return 1;

    return 0;
}

int bp_hit(struct debug_infos *dinfos, struct dproc *proc)
{
    void *bp_addr = get_stopped_addr(proc);
    if (bp_addr == BP_ERR)
        return -1;

    struct breakpoint *bp = bp_htable_get(bp_addr, dinfos->bp_table);
    if (!bp) {
        if (has_hwbp(proc)) // Hit watchpoint
            return hit_watchpoint(dinfos, proc);

        return -1;
    }

    if (bp->state == BP_HIT || bp->state == BP_DISABLED)
        return -1;

    return bp_handlers[bp->type](dinfos, bp, proc);
}
