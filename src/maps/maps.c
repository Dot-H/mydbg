#include <string.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "maps.h"

static FILE *open_maps(pid_t pid)
{
    char maps_path[PATH_MAX_LEN];
    sprintf(maps_path, "/proc/%d/maps", pid);

    FILE *maps = fopen(maps_path, "r");
    if (!maps) {
        warn("Could not open %s\n", maps_path);
        return NULL;
    }

    return maps;
}

int get_offsets(struct map *mapd_line)
{
    char *saveptr;
    char *tok = strtok_r(mapd_line->line, "-", &saveptr);
    mapd_line->ofts[0] = 0;

    for (int i = 1; i < NOFTS; ++i) {
        tok = strtok_r(NULL, " ", &saveptr);
        if (!tok)
            return -1;

        mapd_line->ofts[i] = tok - mapd_line->line;
    }

    /* Test for inode different from 0 */
    if (mapd_line->line[mapd_line->ofts[5]] == '0')
        return -1;

    return 0;
}

struct map *map_line(FILE *maps)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    nread = getline(&line, &len, maps);
    if (nread == -1) {
        free(line);
        return END;
    }

    line[nread - 1] = '\0'; // Replace '\n'
    struct map *mapd_line = malloc(sizeof(struct map));
    if (!mapd_line)
        err(1, "Could not allocate mapd_line");

    mapd_line->line = line;
    if (get_offsets(mapd_line) == -1) {
        map_destroy(mapd_line);
        return NULL;
    }

    return mapd_line;
}

int parse_maps(struct htable *maps_table, pid_t pid)
{
    FILE *maps = open_maps(pid);
    if (!maps)
        return -1;

    struct map *line = map_line(maps);
    while (line != END) {
        if (line && map_htable_insert(line, maps_table) == -1)
            map_destroy(line);
        else
            if (line)
                printf("Insert: %s\n", line->line + line->ofts[NOFTS - 1]);
        line = map_line(maps);
    }

    fclose(maps);
    return 0;
}


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

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;

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

            wl_list_remove(&tmp->link);
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

struct map *map_htable_get(char *name, struct htable *htable)
{
    struct data *map = htable_get(htable, name);
    if (!map)
        return NULL;

    return map->value;
}

void map_htable_remove(struct map *map, struct htable *htable)
{
    struct data *poped = htable_pop(htable, map->line + map->ofts[NOFTS - 1]);
    if (!poped)
        fprintf(stderr, "Failed to find %s in map hashtable\n",
                map->line + map->ofts[NOFTS - 1]);

    map_destroy(poped->value);
    free(poped);
}

int map_htable_insert(struct map *map, struct htable *htable)
{
    int ret = htable_insert(htable, map, map->line + map->ofts[NOFTS - 1]);
    if (ret == -1){
        fprintf(stderr, "Ignored %s\n", map->line + map->ofts[NOFTS - 1]);
    }
 
    return ret;
}
