#include <sys/mman.h>
#include <stdio.h>

#include "mapping.h"

int main(int argc, char *argv[])
{
    if (argc != 2){
        fprintf(stderr, "Usage: ./my_dbg elf\n");
        return 2;
    }

    size_t size = 0;
    void *elf = map_elf(argv[1], &size);
    if (!elf)
        return 1;

    munmap(elf, size);
    return 0;
}
