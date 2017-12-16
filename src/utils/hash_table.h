#ifndef HASH_TABLE_H
# define HASH_TABLE_H

# include "wayland-util.h"

struct data {
   void *key; 
   void *elt;
   struct wl_list link;
};

struct hash_table
{
    size_t nmemb; /* Number of elements in the table and collision lists */
    size_t size;  /* Current number of cell in the array */

    size_t (*hash_func)(void *); /* Function used to hash the key */
    int (*key_cmp)(void *, void *); /* Function used to compare the key */
    struct data *array; /* Array of sentinels to the data */
};

struct hash_table *hash_table_creat(size_t (*hash_func)(void *), size_t size,
                                    int (*key_cmp)(void *, void *));

void hash_table_destroy(struct hash_table *hash_table);

struct data *hash_table_get(const struct hash_table *hash_table, void *key);

struct data *hash_table_pop(struct hash_table *hash_table, void *key);

/*
** Push front a new pair list at the index given by the hash of
** pointer value.
**
** Returns true on success and false on failure.
*/
int hash_table_insert(struct hash_table *hash_table, void *value, void *key);

void hash_table_dump(struct hash_table *hash);

#endif /* !HASH_TABLE_H */
