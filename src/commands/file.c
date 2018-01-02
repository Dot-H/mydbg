#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "commands.h"
#include "inputs.h"
#include "mapping.h"
#include "my_dbg.h"

/**
** \brief Change the working directory to the directory where \p file is.
**
** \note This operation is time and memory saving when debugging a file
** with debugging informations. It allows the debugger to not search and
** save the directories of source files which are in the same working
** directory than the debugged file.
*/
static void change_dir(char *file)
{
    if (!file[0])
        return;

    size_t len = strlen(file);
    while (file[len - 1] != '/' && len > 0)
        --len;

    char svd  = file[len];
    file[len] = '\0';

    if (chdir(file) == -1)
        warn("Could not change the directory to %s, you still can debug but \
you may encounter problems while printing source code.", file);

    file[len] = svd;
}

int load_file(struct debug_infos *dinfos, char *args[])
{
    if (check_params(args, 2, -1) == -1)
        return -1;

    char *file = realpath(args[1], NULL);
    if (!file) {
        fprintf(stderr, "%s: No such file\n", args[1]);
        return -1;
    }

    fprintf(stderr, "loading %s ...\n", file);
    fflush(stdout);

    empty_debug_infos(dinfos);
    size_t size = 0;
    void *elf   = map_elf(args[1], &size);
    if (!elf) {
        free(file);
        return -1;
    }

    dinfos->melf.elf  = elf;
    dinfos->melf.size = size;
    dinfos->args      = dup_args(args + 1);
    free(dinfos->args[0]);
    dinfos->args[0] = file;
    get_symbols(&dinfos->melf);
    change_dir(dinfos->args[0]);

    fprintf(stderr, "done\n");
    return 0;
}

shell_cmd(file, load_file, "Load file given in argument. Replace the current \
debugged file if any");
