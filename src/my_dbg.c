#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "my_dbg.h"
#include "inputs.h"
#include "commands.h"
#include "breakpoint.h"
#include "dproc.h"
#include "maps.h"


struct debug_infos *init_debug_infos(void)
{
    struct debug_infos *dinfos = malloc(sizeof(struct debug_infos));
    if (!dinfos)
        err(1, "Cannot allocate struct debug_infos");

    dinfos->dflt_pid      = 0;
    dinfos->melf.elf      = NULL;
    dinfos->melf.size     = 0;
    dinfos->melf.dw_table = NULL;
    dinfos->args          = NULL;
    dinfos->dproc_table   = dproc_htable_creat();
    dinfos->bp_table      = bp_htable_creat();
    dinfos->maps_table    = map_htable_creat();

    return dinfos;
}

void empty_debug_infos(struct debug_infos *dinfos)
{

    bp_htable_reset(dinfos, dinfos->bp_table);
    destroy_args(dinfos->args);
    map_htable_reset(dinfos->maps_table);
    dproc_htable_reset(dinfos->dproc_table);

    reset_melf(&dinfos->melf);
    dinfos->args     = NULL;
    dinfos->dflt_pid = 0;
}

void destroy_debug_infos(struct debug_infos *dinfos)
{
    if (dinfos->melf.elf && munmap(dinfos->melf.elf, dinfos->melf.size) == -1)
        warn("Cannot unmap %p", dinfos->melf.elf);

    empty_debug_infos(dinfos);
    bp_htable_destroy(dinfos, dinfos->bp_table);
    map_htable_destroy(dinfos->maps_table);
    dproc_htable_destroy(dinfos->dproc_table);
    free(dinfos);
}

static int interaction(struct debug_infos *dinfos)
{
    int ret = 0;
    init_interaction();

    char *line = NULL;
    while ((line = get_line())) {
        char **args = build_cmd(line);
        if (!args)
            goto cont_free;

        struct command *cmd = find_command(args[0]);
        if (!cmd)
        {
            fprintf(stderr, "Command not found\n");
            ret = 1;
        }
        else
            ret = cmd->func(dinfos, args);

cont_free:
        free(args);
        free(line);
    }

    destroy_debug_infos(dinfos);
    fprintf(stderr, QUIT_MSG"\n");
    return ret;
}

int main(int argc, char *argv[])
{
    if (argc > 2){
        fprintf(stderr, "Usage: ./my_dbg elf\n");
        return 2;
    }

    struct debug_infos *dinfos = init_debug_infos();
    if (argc == 2)
        load_file(dinfos, argv);

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
