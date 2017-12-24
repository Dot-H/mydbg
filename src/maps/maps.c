#include <string.h>

#include "maps.h"

void map_destroy(struct map *map)
{
    free(map->line);
    free(map);
}

/**
** djb2 algorithm first reported by dan bernstein
** Hash function from http://www.cse.yorku.ca/~oz/hash.html
*/
static size_t map_hash(void *name)
{

    unsigned char *str = name;
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static int map_cmp(void *namea, void *nameb)
{
    return !strcmp(namea, nameb);
}

struct htable *map_htable_creat(void)
{
   return htable_creat(map_hash, MAP_HTABLE_SIZE, map_cmp);
}

void map_htable_reset(struct htable *htable)
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
            map_destroy(tmp->value);
            free(tmp);
            ++j;
        }
    }

    htable->nmemb = 0;
}

void map_htable_destroy(struct htable *htable)
{
    map_htable_reset(htable);

    free(htable->array);
    free(htable);
}

struct map *map_htable_get(const char *name, struct htable *htable)
{
    struct data *map = htable_get(htable, name);
    if (!map)
        return NULL;

    return map->value;
}

void map_htable_remove(struct map *map, struct htable *htable)
{
    struct data *poped = htable_pop(htable, &map->pid);
    if (!poped)
        fprintf(stderr, "Failed to find %d in map hashtable\n", map->pid);

    map_destroy(poped->value);
    free(poped);
}

int map_htable_insert(struct map *map, struct htable *htable)
{
    int ret = htable_insert(htable, map, &map->pid);
    if (ret == -1){
        fprintf(stderr, "Process %d is already present in the hashtable",
                map->pid);
    }

    return ret;
}
