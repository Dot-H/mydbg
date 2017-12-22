#include <err.h>
#include <stdlib.h>

#include "hash_table.h"


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
    h->array     = malloc(size * sizeof(struct data));
    if (!h->array)
        err(1, "Failed to allocate the htable's array of size %zu", size);

    for (size_t i = 0; i < size; ++i){
        h->array[i].key = NULL;
        h->array[i].value = NULL;
        wl_list_init(&h->array[i].link);
    }

    return h;
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

    return 0;
}


#if 0
__attribute__((__visibility__("hidden")))
void *htable_realloc(struct htable *htable, void *ptr,
                         size_t size)
{
  struct pair_list *tmp = htable_get(htable, ptr);
  assert(tmp);

  size_t page_size = sysconf(_SC_PAGESIZE);
  size = (my_log2(size, page_size) + 1) * sysconf(_SC_PAGESIZE);

  size_t old_size = (tmp->size & ~(MAX_SIZE))  * page_size;
  void *new = mremap(ptr, old_size, size, MREMAP_MAYMOVE);

  if (new == MAP_FAILED)
    return NULL;

  if (tmp != new)
  {
    htable_pop(htable, ptr, 0);
    htable_insert(htable, new, size);
    return new;
  }

  tmp->size  = size;

  return new;
}
#endif
