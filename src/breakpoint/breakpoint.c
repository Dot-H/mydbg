#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#include "breakpoint.h"
#include "my_dbg.h"
#include "trace.h"

static size_t bp_id = 1;

struct breakpoint *bp_creat(enum bp_type type)
{
    struct breakpoint *new = calloc(1, sizeof(struct breakpoint));
    if (!new)
        err(1, "Cannot allocate memory for struct breakpoint");

    new->type = type;
    return new;
}

int bp_set(struct debug_infos *dinfos, struct breakpoint *bp,
           void *bp_addr, pid_t pid)
{
    bp->a_pid = pid;
    bp->sv_instr = set_opcode(bp->a_pid, BP_OPCODE, bp_addr);
    if (bp->sv_instr == -1)
        goto out_destroy_bp;

    bp->addr  = bp_addr;
    bp->state = BP_ENABLED;

    if (bp_htable_insert(bp, dinfos->bp_table) == -1)
        goto out_destroy_bp;

    return 0;

out_destroy_bp:
    bp_destroy(bp);
    return -1;
}

int bp_destroy(struct breakpoint *bp)
{
    if (!bp)
        return -1;

    if (bp->type != BP_RESET && bp->sv_instr && bp->sv_instr != -1)
        if (set_opcode(bp->a_pid, bp->sv_instr, bp->addr) == -1)
            return -1;

    free(bp);
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
    if (bp)
        return bp_reset(dinfos, bp, proc);

    return 0;
}

/**
** Hash function from an article of Thomas Wang, Jan 1997.
** https://gist.github.com/badboy/6267743
*/
static size_t addr_hash(void *addr)
{
    uintptr_t key = (uintptr_t)addr;

    key = (~key) + (key << 21); // key = (key << 21) - key - 1;
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8); // key * 265
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4); // key * 21
    key = key ^ (key >> 28);
    key = key + (key << 31);

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

void bp_htable_reset(struct htable *htable)
{
    for (size_t i = 0, j = 0; i < htable->size && j < htable->nmemb; ++i)
    {
        struct wl_list *head = &htable->array[i].link;
        struct data *pos    = wl_container_of(head->next, pos, link);
        while (&pos->link != head)
        {
            struct data *tmp = pos;
            pos = wl_container_of(pos->link.next, pos, link);

            if (bp_destroy(tmp->value)) /* If it fails, we are screwed */
                wl_list_remove(&tmp->link);

            free(tmp);
            ++j;
        }
    }

    htable->nmemb = 0;
    bp_id         = 1;
}

void bp_htable_destroy(struct htable *htable)
{
    bp_htable_reset(htable);

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

void bp_htable_remove(struct breakpoint *bp, struct htable *htable)
{
    struct data *poped = htable_pop(htable, bp->addr);
    if (!poped)
        fprintf(stderr, "Failed to find %p in htable\n", bp->addr);

    bp_destroy(poped->value);
    free(poped);
}

int bp_htable_insert(struct breakpoint *bp, struct htable *htable)
{
    int ret = htable_insert(htable, bp, bp->addr);
    if (ret == -1){
        fprintf(stderr, "A breakpoint is already set at %p\n", bp->addr);
        return -1;
    }

    if (bp->type != BP_RESET) {
        bp->id = bp_id;
        ++bp_id;
    }

    if (bp->type != BP_RESET)
        printf("Breakpoint %d set at %p\n", bp->id, bp->addr);

    return ret;
}
