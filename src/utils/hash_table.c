#include <err.h>
#include <stdlib.h>

#include "hash_table.h"

static struct data *init_array(size_t size)
{
    struct data *array = malloc(size * sizeof(struct data));
    if (!array) {
        warn("Failed to allocate the htable's array of size %zu", size);
        return NULL;
    }

    for (size_t i = 0; i < size; ++i){
        array[i].key = NULL;
        array[i].value = NULL;
        wl_list_init(&array[i].link);
    }

    return array;
}

struct htable *htable_creat(size_t (*hash_func)(void *), size_t size,
                                    int (*key_cmp)(void *, void *))
{
    struct htable *h = malloc(sizeof(struct htable));
    if (!h)
        err(1, "Failed to allocate a struct htable");

    h->nmemb     = 0;
    h->size      = size;
    h->hash_func = hash_func;
    h->key_cmp   = key_cmp;
    h->array     = init_array(size);
    if (!h->array) /* Catastrophic */
        exit(1);

    return h;
}

void htable_extend(struct htable *htable)
{
    htable->size += htable->size / 2 + 1;
    struct data *newarray = init_array(htable->size);
    if (!newarray) /* Not catastrophic */
        return;

    for (size_t i = 0, j = 0; i < htable->size && j < htable->nmemb; ++i)
    {
        struct wl_list *head = &htable->array[i].link;
        struct data *pos    = wl_container_of(head->next, pos, link);
        while (&pos->link != head)
        {
            struct data *tmp = pos;
            pos = wl_container_of(pos->link.next, pos, link);
            wl_list_remove(&tmp->link);

            size_t idx = htable->hash_func(tmp->key) % htable->size;
            wl_list_insert(&newarray[idx].link, &tmp->link);
            ++j;
        }
    }

    free(htable->array);
    htable->array = newarray;
}

struct data *htable_get(const struct htable *htable, void *key)
{
    size_t idx           = htable->hash_func(key) % htable->size;
    struct wl_list *head = &htable->array[idx].link;

    struct data *search;
    wl_list_for_each(search, head, link){
        if (htable->key_cmp(search->key, key))
            break;
    }

    if (&search->link == head)
        return NULL;

    return search;
}

struct data *htable_pop(struct htable *htable, void *key)
{
    struct data *search = htable_get(htable, key);
    if (!search)
        return NULL;

    htable->nmemb -= 1;
    wl_list_remove(&search->link);

    return search;
}

static struct data *data_creat(void *value, void *key)
{
  struct data *new = malloc(sizeof(struct data));
  if (!new)
      err(1, "Cannot create a data node");

  new->value = value;
  new->key = key;
  wl_list_init(&new->link);

  return new;
}

int htable_insert(struct htable *htable, void *value, void *key)
{
    size_t idx = htable->hash_func(key) % htable->size;
    struct data *search = htable_get(htable, key);
    if (search)
        return -1;

    struct data *new = data_creat(value, key);
    wl_list_insert(&htable->array[idx].link, &new->link);
    htable->nmemb += 1;

    float pop = (float)htable->nmemb / (float)htable->size;
    if (pop >= 0.70)
        htable_extend(htable);

    return 0;
}
