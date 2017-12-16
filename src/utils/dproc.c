#include <err.h>
#include <stdlib.h>
#include <stdio.h>

#include "dproc.h"

struct dproc *dproc_creat(pid_t pid, int status)
{
    struct dproc *new = malloc(sizeof(struct dproc));
    if (!new)
        err(1, "Cannot allocate space for struct dproc");

    new->pid    = pid;
    new->status = status;
    return new;
}

void dproc_destroy(struct dproc *proc)
{
    free(proc);
}

/**
** Hash function from an article of Thomas Wang, Jan 1997.
** https://gist.github.com/badboy/6267743
*/
static size_t pid_hash(void *pid)
{
    pid_t hpid = *(pid_t *)pid;

    hpid = (hpid+0x7ed55d16) + (hpid<<12);
    hpid = (hpid^0xc761c23c) ^ (hpid>>19);
    hpid = (hpid+0x165667b1) + (hpid<<5);
    hpid = (hpid+0xd3a2646c) ^ (hpid<<9);
    hpid = (hpid+0xfd7046c5) + (hpid<<3);
    hpid = (hpid^0xb55a4f09) ^ (hpid>>16);

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

void dproc_htable_destroy(struct htable *htable)
{
    for (size_t i = 0, j = 0; i < htable->size && j < htable->nmemb; ++i)
    {
        struct wl_list head = htable->array[i].link;
        struct data *pos    = wl_container_of(head.next, pos, link);
        while (&pos->link != &head)
        {
            struct data *tmp = pos;
            pos = wl_container_of(pos->link.next, pos, link);
            dproc_destroy(tmp->value);
            free(tmp);
            ++j;
        }
    }

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
