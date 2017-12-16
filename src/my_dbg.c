#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "my_dbg.h"
#include "inputs.h"
#include "mapping.h"
#include "commands.h"
#include "trace.h"


struct debug_infos *init_debug_infos(char *const *args, void *elf,
                                     size_t size)
{
    struct debug_infos *dinfos = malloc(sizeof(struct debug_infos));
    if (!dinfos)
        err(1, "Cannot allocate struct debug_infos");

    dinfos->melf.elf  = elf;
    dinfos->melf.size = (elf) ? size : 0;
    dinfos->args = args;

    return dinfos;
}

void empty_debug_infos(struct debug_infos *dinfos)
{
    if (munmap(dinfos->melf.elf, dinfos->melf.size) == -1)
        warn("Cannot unmap %p", dinfos->melf.elf);

    memset(dinfos, 0, sizeof(struct debug_infos));
}

void destroy_debug_infos(struct debug_infos *dinfos)
{
    if (munmap(dinfos->melf.elf, dinfos->melf.size) == -1)
        warn("Cannot unmap %p", dinfos->melf.elf);

    free(dinfos);
}

static int interaction(struct debug_infos *dinfos)
{
    int ret = 0;
    init_interaction();
    if (dinfos->melf.elf)
        trace_binary(dinfos);

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
    return ret;
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

    struct debug_infos *dinfos = init_debug_infos(argv + 1, elf, size);

    int ret = 0;
    pid_t fk = fork();
    if (fk == -1)
        return 1;
    else if (!fk)
        exit(interaction(dinfos));

    int status;
    if (waitpid(fk, &status, 0) == -1)
        err(1, "Failed to wait %d", fk);

    ret = WEXITSTATUS(status);

    destroy_debug_infos(dinfos);
    return ret;
}
