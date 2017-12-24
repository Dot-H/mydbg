#ifndef MAPS_H
# define MAPS_H

# include <stdint.h>

# include "hash_table.h"

# define MAP_HTABLE_SIZE 64
# define PERM 4
# define DEVICE 5

/*
** A struct storing each element is too big (+-8 48bytes).
** Even if I need to strtol the offsets each time I need it,
** it will rarely happend and not in a time-critic situation.
*/
struct map {
    char *line; /* Contains the line */
    uint8_t ofts[7]; /* Ofts:
                    1: start
                    2: end
                    3: perms
                    4: oft
                    5: bloc
                    6: inode
                    7: lib
                  */
};

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

struct map *map_htable_get(pid_t pid, struct htable *htable);

void map_htable_remove(struct map *map, struct htable *htable);

int map_htable_insert(struct map *map, struct htable *htable);

#endif /* !MAPS_H */
