#include <stdio.h>

#include "commands.h"

int print_help(struct debug_infos *dinfos, const char *args[])
{
    (void)dinfos;
    (void)args;
    size_t len = __stop_cmds - __start_cmds;

    for (size_t i = 0; i < len; ++i) {
        struct command *cmd = __start_cmds + i;
        printf("%s: %s\n", cmd->name, cmd->doc);
    }

    return 0;
}

shell_cmd(help, print_help, "Display this text");
