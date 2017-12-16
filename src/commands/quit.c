#include <stdio.h>
#include <stdlib.h>

#include "commands.h"
#include "my_dbg.h"

int do_quit(struct debug_infos *dinfos, char *args[])
{
    (void)args;

    destroy_debug_infos(dinfos);
    fprintf(stderr, QUIT_MSG"\n");
    exit(0);
}

shell_cmd(quit, do_quit, "Quit mydbg");
