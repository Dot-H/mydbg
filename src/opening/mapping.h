#ifndef MAPPING_H_
# define MAPPING_H_

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

#endif /* !MAPPING_H_ */
