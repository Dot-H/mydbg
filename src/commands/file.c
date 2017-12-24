#include <stdio.h>
#include <stdlib.h>

#include "commands.h"
#include "inputs.h"
#include "mapping.h"
#include "my_dbg.h"

int load_file(struct debug_infos *dinfos, char *args[])
{
    if (check_params(args, 2, 2) == -1)
        return -1;

    fprintf(stderr, "loading %s ...", args[1]);
    fflush(stdout);

    empty_debug_infos(dinfos);
    size_t size = 0;
    void *elf   = map_elf(args[1], &size);
    if (!elf)
        return -1;

    dinfos->melf.elf  = elf;
    dinfos->melf.size = size;
    dinfos->args      = dup_args(args + 1);

    fprintf(stderr, "done\n");
    find_symbol(dinfos->melf.elf);
    return 0;
}

shell_cmd(file, load_file, "Load file given in argument. Replace the current \
debugged file if any.");
