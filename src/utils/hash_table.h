#ifndef HASH_TABLE_H
# define HASH_TABLE_H

# include "wayland-util.h"

struct data {
   void *key; /* Should be an element inside value */
   void *value;
   struct wl_list link;
};

struct htable
{
    size_t nmemb; /* Number of elements in the table and collision lists */
    size_t size;  /* Current number of cell in the array */

    size_t (*hash_func)(void *); /* Function used to hash the key */
    int (*key_cmp)(void *, void *); /* Function used to compare the key */
    struct data *array; /* Array of sentinels to the data */
};

/**
** \param hash_func function pointer to the hashing function which
** will be used in the htable.
** \param key_cmp function pointer to the function comparing keys.
** This function must return 1 if keys match and 0 otherwise.
** \param size size of the htable
**
** \brief allocates and fill a struct htable and its array.
**
** \note each case of the array attribute is set as the sentinell
** for its collision list.
**
** \return Returns a newly allocated and filled hash table.
*/
struct htable *htable_creat(size_t (*hash_func)(void *), size_t size,
                                    int (*key_cmp)(void *, void *));

/**
** \param htable struct to destroy.
**
** \brief Destroys all the allocated memory inside \p htable. 
*/
void htable_destroy(struct htable *htable);

/**
** \param key Key to search inside \p htable
**
** \brief Search a matching key inside the htable.
**
** \return Return the data corresponding to the key if found and
** NULL if the key is not present in \p htable.
*/
struct data *htable_get(const struct htable *htable, void *key);

/**
** \param key Key to remove from \p htable
**
** \brief Search key and removes it from its corresponding
** collision list.
**
** \return Return the data corresponding to the key if found and NULL
** otherwise.
*/
struct data *htable_pop(struct htable *htable, void *key);

/**
** \param key key corresponding to \p value
** \param value value to insert in \p htable
**
** \brief Insert \p value inside \p htable at the head of the
** collision list corresponding to the hash of \p key
**
** \return Return -1 if the key is already present in the htable
** and 0 if the value has been successfully inserted into \p htable
*/
int htable_insert(struct htable *htable, void *value, void *key);

#endif /* !HASH_TABLE_H */
