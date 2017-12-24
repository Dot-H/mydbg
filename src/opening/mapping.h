#ifndef MAPPING_H
# define MAPPING_H

# include <elf.h>

struct melf {
    void  *elf;
    size_t size;
};

struct gnu_table {
    uint32_t nbuckets;
    uint32_t symoffset;
    uint32_t bloom_size;
    uint32_t bloom_shift;
    uint64_t bloom;
};

/**
** \fn void *get_mapped_elf(struct stat stat, int fd)
** \param file File to map.
** \param size Size of the returned mapping.
** \brief Map the file given in argument and fills the size
** argument with the size of the mapping.
** \return Returns a pointer to the mapped area on success
** and NULL in case of error.
*/
void *map_elf(const char *file, size_t *size);

const Elf64_Sym *find_symbol(Elf64_Ehdr *header);

#endif /* !MAPPING_H */
