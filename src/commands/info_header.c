#include <elf.h>
#include <stdio.h>
#include <stdlib.h>

#include "commands.h"
#include "inputs.h"
#include "mapping.h"
#include "my_dbg.h"

int print_header(struct debug_infos *dinfos, char *args[])
{
    if (check_params(args, 1, 1) == -1)
        return -1;

    if (!dinfos->melf.elf){
        fprintf(stderr, "No elf loaded\n");
        return -1;
    }

    Elf64_Ehdr *header = dinfos->melf.elf;

    fprintf(stderr, "done\n");

    const char *type = "ET_NONE";
    if (header->e_type == ET_DYN) type = "ET_DYN";
    else if (header->e_type == ET_REL) type = "ET_REL";
    else if (header->e_type == ET_EXEC) type = "ET_EXEC";
    else if (header->e_type == ET_CORE) type = "ET_CORE";
    printf("type: %s\n", type);
    printf("entrypoint: 0x%lx\n", header->e_entry);
    printf("header's size: %hu\n", header->e_ehsize);
    printf("phentsize: %hu\n", header->e_phentsize);
    printf("phnum: %hu\n", header->e_phnum);
    printf("shentsize: %hu\n", header->e_shentsize);
    printf("shnum: %hu\n", header->e_shnum);
    printf("shstrnidx: %hu\n", header->e_shentsize);
    fflush(stdout);

    return 0;
}

shell_cmd(info_header, print_header, "Print info in elf header");
