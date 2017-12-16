#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "my_dbg.h"
#include "inputs.h"
#include "mapping.h"
#include "commands.h"


struct debug_infos *init_debug_infos(void *elf, size_t size)
{
    struct debug_infos *dinfos = malloc(sizeof(struct debug_infos));
    if (!dinfos)
        err(1, "Cannot allocate struct debug_infos");

    dinfos->melf.elf  = elf;
    dinfos->melf.size = size;

    return dinfos;
}

int destroy_debug_infos(struct debug_infos *dinfos)
{
    if (munmap(dinfos->melf.elf, dinfos->melf.size) == -1){
        warn("Cannot unmap %p", dinfos->melf.elf);
        return 1;
    }

    free(dinfos);
    return 0;
}

static void interaction(struct debug_infos *dinfos)
{
    int ret = 0;

    init_interaction();

    char *line = NULL;
    while ((line = get_line())) {
        char *stripped = strip_whitespace(line);
        struct command *cmd = find_command(stripped);
        if (!cmd)
        {
            fprintf(stderr, "Command not found\n");
            ret = 1;
        }
        else
            ret = cmd->func(dinfos, NULL);

        free(line);
    }

    fprintf(stderr, QUIT_MSG"\n");
    exit(ret);
}

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

    struct debug_infos *dinfos = init_debug_infos(elf, size);

    int ret = 0;
    int fk = fork();
    if (fk == -1)
        return 1;
    else if (!fk)
        interaction(dinfos);
    else{
        int status;
        if (waitpid(fk, &status, 0) == -1)
            err(1, "Failed to wait %d", fk);

        ret = WEXITSTATUS(status);
    }

    destroy_debug_infos(dinfos);
    return ret;
}
