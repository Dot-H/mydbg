#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "my_dbg.h"
#include "breakpoint.h"

static size_t bp_id = 1;

struct breakpoint *bp_creat(enum bp_type type)
{
    struct breakpoint *new = calloc(1, sizeof(struct breakpoint));
    if (!new)
        err(1, "Cannot allocate space for struct breakpoint");

    new->type = type;
    return new;
}

void bp_destroy(struct breakpoint *bp)
{
    if (!bp)
        return;

    free(bp);
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
            bp_destroy(tmp->value);
            free(tmp);
            ++j;
        }
    }

    htable->nmemb = 0;
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
        fprintf(stderr, "Failed to find %p in breakpoint hashtable\n", bp->addr);

    free(poped);
}

int bp_htable_insert(struct breakpoint *bp, struct htable *htable)
{
    int ret = htable_insert(htable, bp, &bp->addr);
    if (ret == -1){
        fprintf(stderr, "Process %p is already present in the hashtable",
                bp->addr);
    }

    bp->id = bp_id;
    ++bp_id;

    return ret;
}
