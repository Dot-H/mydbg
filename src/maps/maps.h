#ifndef MAPS_H
# define MAPS_H

# include <stdint.h>
# include <sys/types.h>

# include "hash_table.h"

# define MAP_HTABLE_SIZE 64

/* Used for parsing */
# define PATH_MAX_LEN 23
# define END (void *)-1
# define NOFTS 7


/*
** A struct storing each element is too big (+-8 48bytes).
** Even if I do strtol the offsets each time I need it,
** it will rarely happend and not in a time-critic situation.
*/
struct map {
    char *line; /* Contains the line */
    uint8_t ofts[NOFTS]; /* Ofts:
                          0: start
                          1: end
                          2: perms
                          3: oft
                          4: bloc
                          5: inode
                          6: file
                         */
};

/**
** \brief free all the allocated memory inside map. Including
** itself.
*/
void map_destroy(struct map *map);

struct htable *parse_maps(pid_t pid);

/****************************************/
/*      Wrappers to struct htable       */
/****************************************/

/**
** \brief Call the htable_creat function with map_hash, map_cmp and
** MAP_HTABLE_SIZE.
*/
struct htable *map_htable_creat(void);

/**
** \brief Free all the allocated memory inside \p htable and \p htable
** itself.
*/
void map_htable_destroy(struct htable *htable);

void map_htable_reset(struct htable *htable);

struct map *map_htable_get(char *name, struct htable *htable);

void map_htable_remove(struct map *map, struct htable *htable);

int map_htable_insert(struct map *map, struct htable *htable);


#endif /* !MAPS_H */
