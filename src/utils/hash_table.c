#include <err.h>
#include <stdlib.h>

#include "hash_table.h"


struct hash_table *hash_table_creat(size_t (*hash_func)(void *), size_t size,
                                    int (*key_cmp)(void *, void *))
{
    struct hash_table *h = malloc(sizeof(struct hash_table));
    if (!h)
        err(1, "Failed to allocate a struct hash_table");

    h->nmemb     = 0;
    h->size      = size;
    h->hash_func = hash_func;
    h->key_cmp   = key_cmp;
    h->array     = malloc(size * sizeof(struct data));
    if (!h->array)
        err(1, "Failed to allocate the hash_table's array of size %zu", size);

    for (size_t i = 0; i < size; ++i){
        h->array[i].key = NULL;
        h->array[i].elt = NULL;
        wl_list_init(&h->array[i].link);
    }

    return h;
}

void hash_table_destroy(struct hash_table *hash_table)
{
    (void)hash_table;
    /* TODO */
}

struct data *hash_table_get(const struct hash_table *hash_table, void *key)
{
    size_t idx       = hash_table->hash_func(key) % hash_table->size;
    struct data head = hash_table->array[idx];

    struct data *search;
    wl_list_for_each(search, &head.link, link){
        if (hash_table->key_cmp(search->key, key))
            break;
    }

    if (search == &head)
        return NULL;

    return search;
}

struct data *hash_table_pop(struct hash_table *hash_table, void *key)
{
    struct data *search = hash_table_get(hash_table, key);
    if (!search)
        return NULL;

    wl_list_remove(&search->link);
    return search;
}

static struct data *data_creat(void *elt, void *key)
{
  struct data *new = malloc(sizeof(struct data));
  if (!new)
      err(1, "Cannot create a data node");

  new->elt = elt;
  new->key = key;
  wl_list_init(&new->link);

  return new;
}

int hash_table_insert(struct hash_table *hash_table, void *value, void *key)
{
    size_t idx = hash_table->hash_func(key); 
    struct data *search = hash_table_get(hash_table, key);
    if (search)
        return -1;

    struct data *new = data_creat(value, key);
    wl_list_insert(&hash_table->array[idx].link, &new->link);

    return 0;
}


#if 0
__attribute__((__visibility__("hidden")))
void *hash_table_realloc(struct hash_table *hash_table, void *ptr,
                         size_t size)
{
  struct pair_list *tmp = hash_table_get(hash_table, ptr);
  assert(tmp);

  size_t page_size = sysconf(_SC_PAGESIZE);
  size = (my_log2(size, page_size) + 1) * sysconf(_SC_PAGESIZE);

  size_t old_size = (tmp->size & ~(MAX_SIZE))  * page_size; 
  void *new = mremap(ptr, old_size, size, MREMAP_MAYMOVE);

  if (new == MAP_FAILED)
    return NULL;

  if (tmp != new)
  {
    hash_table_pop(hash_table, ptr, 0);
    hash_table_insert(hash_table, new, size);
    return new;
  }

  tmp->size  = size;

  return new;
}
#endif
