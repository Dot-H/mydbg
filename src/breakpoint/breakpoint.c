#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#include "breakpoint.h"
#include "my_dbg.h"
#include "trace.h"

static size_t bp_id = 1;

/**
** Garbage counter for syscall breakpoints. Permits the bp wrappers
** to handle the ptrace request set in the struct debug_infos.
*/
static size_t bp_nsys = 0;

struct breakpoint *bp_creat(enum bp_type type)
{
    struct breakpoint *new = calloc(1, sizeof(struct breakpoint));
    if (!new)
        err(1, "Cannot allocate memory for struct breakpoint");

    if (type == BP_SYSCALL)
        ++bp_nsys;

    new->type  = type;
    return new;
}


int bp_hw_poke(struct debug_infos *dinfos, struct breakpoint *bp,
               enum bp_hw_cond cond, enum bp_hw_len len)
{
    struct dproc *proc = dproc_htable_get(bp->a_pid, dinfos->dproc_table);
    if (!proc) {
        fprintf(stderr, "Could not find process %d\n", bp->a_pid);
        return -1;
    }

    int i = 0;
    while (i < BP_NHW && proc->bp_hwtab[i])
        ++i;

    if (i == BP_NHW)
        return -BP_NHW;

    long dr7_tst = ptrace(PTRACE_PEEKUSER, bp->a_pid, DR_OFFSET(7), 0);
    if (dr7_tst == -1) {
        warn("Could not get %%dr7 from process %d\n", bp->a_pid);
        return -1;
    }

    struct dr7 *dr7 = (struct dr7 *)(&dr7_tst);
    dr7_set_hwlabel(dr7, local, i, 1)
    dr7_set_hwlabel(dr7, cond, i, cond)
    dr7_set_hwlabel(dr7, len, i, len)
    dr7->local_exact  = 1;
    dr7->global_exact = 1;
    dr7->reserved_1   = 1;

    if (ptrace(PTRACE_POKEUSER, bp->a_pid, DR_OFFSET(i),
               bp->addr) == -1) {
        warn ("Could not put breakpoint's address in %%dr%d", i);
        return -1;
    }

    if (ptrace(PTRACE_POKEUSER, bp->a_pid, DR_OFFSET(7), *dr7) == -1) {
        warn ("Could not setup %%dr7 for %%dr%d", i);
        return -1;
    }

    uintptr_t bp_addr = (uintptr_t)bp->addr;
    bp->addr    = (void *)(bp_addr - 1);
    bp->id      = i;
    proc->bp_hwtab[i] = bp_addr;
    return 0;
}

int bp_set(struct debug_infos *dinfos, struct breakpoint *bp,
           void *bp_addr, pid_t pid)
{
    bp->a_pid = pid;
    bp->addr  = bp_addr;
    bp->state = BP_ENABLED;

    if (bp->type == BP_HARDWARE) {
        int ret;
        if ((ret = bp_hw_poke(dinfos, bp, BP_HW_INSTR, BP_HW_LEN_1BY)) < 0) {
            if (ret == -BP_NHW)
                fprintf(stderr, "Too many hardware breakpoint already put\n");
            goto err_destroy_bp;
        }
    } else {

        bp->sv_instr = set_opcode(bp->a_pid, BP_OPCODE, bp_addr);
        if (bp->sv_instr == -1)
            goto err_destroy_bp;
    }

    if (bp_htable_insert(bp, dinfos->bp_table) == -1)
        goto err_destroy_bp;

    return 0;

err_destroy_bp:
    bp_destroy(dinfos, bp);
    return -1;
}

int bp_destroy(struct debug_infos *dinfos, struct breakpoint *bp)
{
    if (!bp)
        return -1;

    if (bp->type == BP_HARDWARE) {
        if (bp_hw_unset(dinfos, bp->id, bp->a_pid) == -1)
            return -1;
    } else if (bp->sv_instr && bp->sv_instr != -1) {
        if (set_opcode(bp->a_pid, bp->sv_instr, bp->addr) == -1)
            return -1;
    }

    if (bp->type == BP_SYSCALL)
        --bp_nsys;

    free(bp);
    return 0;
}

int bp_hw_unset(struct debug_infos *dinfos, int dr_offset, pid_t pid)
{
    struct dproc *proc = dproc_htable_get(pid, dinfos->dproc_table);
    if (!proc) {
        fprintf(stderr, "Could not find process %d\n", pid);
        return -1;
    }


    long dr7_tst = ptrace(PTRACE_PEEKUSER, pid, DR_OFFSET(7), 0);
    if (dr7_tst == -1) {
        if (errno == ESRCH) // Process finished
            return 0;

        warn("Could not get %%dr%d and unset it", dr_offset);
        return -1;
    }

    struct dr7 *dr7 = (struct dr7 *)(&dr7_tst);
    dr7_set_hwlabel(dr7, local, dr_offset, 0);

    if (ptrace(PTRACE_POKEUSER, pid, DR_OFFSET(7), *dr7) == -1) {
        warn("Could not poke %%dr7 to unset %%dr%d", dr_offset);
        return -1;
    }

    proc->bp_hwtab[dr_offset] = 0;

    return 0;
}

int bp_reset(struct debug_infos *dinfos, struct breakpoint *bp,
             struct dproc *proc)
{
    if (ptrace(PTRACE_SINGLESTEP, proc->pid, 0, 0) == -1) {
        warn("Could not singlestep");
        return -1;
    }

    wait_tracee(dinfos, proc);
    bp->state = BP_ENABLED;

    if (set_opcode(proc->pid, BP_OPCODE, bp->addr) == -1)
        return -1;

    return 0;
}

int bp_cont(struct debug_infos *dinfos, struct dproc *proc)
{
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, proc->pid, 0, &regs) == -1) {
        warn("Could not get registers");
        return -1;
    }

    struct breakpoint *bp = bp_htable_get((void *)regs.rip, dinfos->bp_table);
    int ret = 0;
    if (bp && bp->type != BP_HARDWARE)
        ret = bp_reset(dinfos, bp, proc);

    dinfos->ptrace_req = bp_nsys ? PTRACE_SYSCALL : PTRACE_CONT;
    return ret;
}

/**
** Hash function from an article of Thomas Wang, Jan 1997.
** https://gist.github.com/badboy/6267743
*/
static size_t addr_hash(void *addr)
{
    uintptr_t key = (uintptr_t)addr;

    key = (~key) + (key << 21); // key = (key << 21) - key - 1;
    key = key    ^ (key >> 24);
    key = (key   + (key << 3)) + (key << 8); // key * 265
    key = key    ^ (key >> 14);
    key = (key   + (key << 2)) + (key << 4); // key * 21
    key = key    ^ (key >> 28);
    key = key    + (key << 31);

    return key;
}

static int addr_cmp(void *addra, void *addrb)
{
    return addra == addrb;
}

struct htable *bp_htable_creat(void)
{
   return htable_creat(addr_hash, BP_HTABLE_SIZE, addr_cmp);
}

void bp_htable_reset(struct debug_infos *dinfos, struct htable *htable)
{
    for (size_t i = 0, j = 0; i < htable->size && j < htable->nmemb; ++i)
    {
        struct wl_list *head = &htable->array[i].link;
        struct data *pos    = wl_container_of(head->next, pos, link);
        while (&pos->link != head)
        {
            struct data *tmp = pos;
            pos = wl_container_of(pos->link.next, pos, link);

            if (!bp_destroy(dinfos, tmp->value)) // If it fails, we are screwed
                wl_list_remove(&tmp->link);

            free(tmp);
            ++j;
        }
    }

    htable->nmemb = 0;
    bp_id         = 1;
    bp_nsys       = 0;
}

void bp_htable_destroy(struct debug_infos *dinfos, struct htable *htable)
{
    bp_htable_reset(dinfos, htable);

    free(htable->array);
    free(htable);
}

struct breakpoint *bp_htable_get(void *addr, struct htable *htable)
{
    struct data *bp = htable_get(htable, addr);
    if (!bp)
        return NULL;

    return bp->value;
}

void bp_htable_remove(struct debug_infos *dinfos, struct breakpoint *bp,
                      struct htable *htable)
{
    struct data *poped = htable_pop(htable, bp->addr);
    if (!poped)
        fprintf(stderr, "Failed to find %p in htable\n", bp->addr);

    bp_destroy(dinfos, poped->value);
    free(poped);
}

int bp_htable_remove_by_id(struct debug_infos *dinfos, long id, int is_hw,
                           struct htable *htable)
{
    if (id < 0)
        return -1;

    for (size_t i = 0, j = 0; i < htable->size && j < htable->nmemb; ++i)
    {
        struct wl_list *head = &htable->array[i].link;
        struct data *tmp;
        wl_list_for_each(tmp, head, link) {
            struct breakpoint *bp = tmp->value;
            if ((is_hw && bp->type == BP_HARDWARE)
                || (!is_hw && bp->type != BP_HARDWARE)) {
                if (bp->id == id && !bp_destroy(dinfos, tmp->value)) {
                    wl_list_remove(&tmp->link);
                    free(tmp);
                    htable->nmemb -= 1;
                    return 0;
                }
            }

            ++j;
        }
    }

    return -1;
}

int bp_htable_insert(struct breakpoint *bp, struct htable *htable)
{
    int ret = htable_insert(htable, bp, bp->addr);
    if (ret == -1){
        fprintf(stderr, "A breakpoint is already set at %p\n", bp->addr);
        return -1;
    }

    if (bp->type == BP_SILENT) // No id, no message
        return ret;

    if (bp->type != BP_HARDWARE) { // Id of hardware corresponds to dr offset
        bp->id = bp_id;
        ++bp_id;
    }

    if (bp->type == BP_SYSCALL)
        printf("Breakpoint %d set on syscall %zu\n", bp->id, (size_t)bp->addr);
    else if (bp->type == BP_HARDWARE)
        printf("Hardware breakpoint dr%d set on 0x%lx\n", bp->id,
                                                     (uintptr_t)bp->addr + 1);
    else
        printf("Breakpoint %d set at %p\n", bp->id, bp->addr);

    return ret;
}
