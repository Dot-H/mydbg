#include <elf.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "mapping.h"

#define array_size(t) (sizeof(t) / sizeof(*t))

static int get_fd(const char *file)
{
    int fd = open(file, O_RDONLY);
    if (fd == -1)
        warn("Failed to open %s", file);

    return fd;
}

static void *get_mapped_elf(struct stat stat, int fd)
{
    void *image = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE,
                       fd, 0);

    if (image == MAP_FAILED){
        warn("Cannot map fd %d", fd);
        return NULL;
    }

    return image;
}

/**
** \brief test if the header given in argument corresponds to a
** header from an elf and if the requirede architecture is x86_64.
** If it is not the header is unmap and an  error is print on stderr.
** \return Returns 0 on failure and 1 on success.
*/
static int tst_elf(const char *file, Elf64_Ehdr *header, size_t size)
{
    char magic[] = { 0x7f, 'E', 'L', 'F' };
    size_t len   = array_size(magic);

    for (size_t i = 0; i < len; ++i){
        if (header->e_ident[i] != magic[i]){
            warnx("%s is not an elf", file);
            munmap(header, size);
            return 0;
        }
    }

#ifdef DEBUG
    const char *type = "ET_NONE";
    if (header->e_type == ET_DYN) type = "ET_DYN";
    else if (header->e_type == ET_REL) type = "ET_REL";
    else if (header->e_type == ET_EXEC) type = "ET_EXEC";
    else if (header->e_type == ET_CORE) type = "ET_CORE";
    printf("type: %s\n", type);
    printf("entrypoint: %lx\n", header->e_entry);
    printf("header's size: %hu\n", header->e_ehsize);
    printf("phentsize: %hu\n", header->e_phentsize);
    printf("phnum: %hu\n", header->e_phnum);
    printf("shentsize: %hu\n", header->e_shentsize);
    printf("shnum: %hu\n", header->e_shnum);
    printf("shstrnidx: %hu\n", header->e_shentsize);
    fflush(stdout);
#endif

    return header->e_machine == EM_X86_64;
}

void *map_elf(const char *file, size_t *size)
{
    void *mapped = NULL;

    int fd = get_fd(file);
    if (fd == -1)
        return NULL;

    struct stat stat;
    if (fstat(fd, &stat) == -1){
        warn("Cannot fstat fd %d", fd);
        goto out_close_fd;
    }

    mapped = get_mapped_elf(stat, fd);
    if (!mapped)
        goto out_close_fd;

    if (!tst_elf(file, mapped, stat.st_size))
        mapped = NULL; // unmaped in tst_elf

out_close_fd:
    close(fd);
    *size = stat.st_size;
    return mapped;
}
