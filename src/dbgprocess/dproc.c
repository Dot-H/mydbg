#include <err.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "dproc.h"
#include "breakpoint.h"

static void kill_process(pid_t pid)
{
    if (kill(pid, SIGKILL) == -1)
        warn("Failed to sigkill %d", pid);

    if (waitpid(pid, NULL, 0) == -1)
        warn("Failed to wait %d", pid);

}

struct dproc *dproc_creat(void)
{
    struct dproc *new = calloc(1, sizeof(struct dproc));
    if (!new)
        err(1, "Cannot allocate space for struct dproc");

    new->unw.as = unw_create_addr_space(&_UPT_accessors, 0);
    return new;
}

void dproc_destroy(struct dproc *proc)
{
    if (!proc)
        return;

    if (proc->unw.ui)
        _UPT_destroy(proc->unw.ui);
    if (proc->unw.as)
        unw_destroy_addr_space(proc->unw.as);

    if (proc->pid && !is_finished(proc))
        kill_process(proc->pid);

    free(proc);
}

int is_finished(struct dproc *proc)
{
    return WIFEXITED(proc->status) && WIFSIGNALED(proc->status);
}

char *read_dproc(struct debug_infos *dinfos, struct dproc *proc,
                 size_t size, uintptr_t start_addr)
{
    int word     = sizeof(long);
    size_t alloc = size + word - (size % word) + 1;
    char *dumped = malloc(alloc);

    size_t nb_call = size / word + (size % word != 0);
    size_t i = 0;
    for ( ; i < nb_call; ++i) {
        void *addr   = (void *)(start_addr + i * word);
        long data = ptrace(PTRACE_PEEKTEXT, proc->pid, addr, NULL);
        if (data == -1)
            goto err_free_dumped;

        memcpy(dumped + (i * word), &data, word); 
        for (int j = 0; j < word; ++j) {
            struct breakpoint *bp =
                bp_htable_get((void *)(start_addr + j + i * word),
                              dinfos->bp_table);
            if (bp)
                dumped[i * word + j] = bp->sv_instr & 0xff;
        }
    }


    memset(dumped + size, 0, size % word);
    struct breakpoint *bp = bp_htable_get((void *)(start_addr + 1),
                                          dinfos->bp_table);
    if (bp)
        dumped[1] = bp->sv_instr & 0xff;
    return dumped;

err_free_dumped:
    free(dumped);
    warn("Failed to PEEKTEXT %d at %lx", proc->pid, start_addr + i * word);
    return NULL;
}

/**
** Hash function from an article of Thomas Wang, Jan 1997.
** https://gist.github.com/badboy/6267743
*/
static size_t pid_hash(void *pid)
{
    pid_t hpid = *(pid_t *)pid;

    hpid = (hpid + 0x7ed55d16) + (hpid << 12);
    hpid = (hpid ^ 0xc761c23c) ^ (hpid >> 19);
    hpid = (hpid + 0x165667b1) + (hpid << 5);
    hpid = (hpid + 0xd3a2646c) ^ (hpid << 9);
    hpid = (hpid + 0xfd7046c5) + (hpid << 3);
    hpid = (hpid ^ 0xb55a4f09) ^ (hpid >> 16);

    return hpid;
}

static int pid_cmp(void *pida, void *pidb)
{
    pid_t a = *(pid_t *)pida;
    pid_t b = *(pid_t *)pidb;

    return a == b;
}

struct htable *dproc_htable_creat(void)
{
   return htable_creat(pid_hash, DPROC_HTABLE_SIZE, pid_cmp);
}

void dproc_htable_reset(struct htable *htable)
{
    for (size_t i = 0, j = 0; i < htable->size && j < htable->nmemb; ++i)
    {
        struct wl_list *head = &htable->array[i].link;
        struct data *pos    = wl_container_of(head->next, pos, link);
        while (&pos->link != head)
        {
            struct data *tmp = pos;
            pos = wl_container_of(pos->link.next, pos, link);

            wl_list_remove(&pos->link);
            dproc_destroy(tmp->value);
            free(tmp);
            ++j;
        }
    }

    htable->nmemb = 0;
}

void dproc_htable_destroy(struct htable *htable)
{
    dproc_htable_reset(htable);

    free(htable->array);
    free(htable);
}

struct dproc *dproc_htable_get(pid_t pid, struct htable *htable)
{
    struct data *proc = htable_get(htable, &pid);
    if (!proc)
        return NULL;

    return proc->value;
}

void dproc_htable_remove(struct dproc *proc, struct htable *htable)
{
    struct data *poped = htable_pop(htable, &proc->pid);
    if (!poped)
        fprintf(stderr, "Failed to find %d in dproc hashtable\n", proc->pid);

    dproc_destroy(poped->value);
    free(poped);
}

int dproc_htable_insert(struct dproc *proc, struct htable *htable)
{
    int ret = htable_insert(htable, proc, &proc->pid);
    if (ret == -1){
        fprintf(stderr, "Process %d is already present in the hashtable",
                proc->pid);
    }

    return ret;
}
