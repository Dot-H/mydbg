#include <stdio.h>
#include "commands.h"

int help(struct debug_infos *dinfos, char *args[])
{
    size_t len = __stop_cmds - __start_cmds;

    for (size_t i = 0; i < len; ++i) {
        struct command *cmd = __start_cmds + i;
        printf("%s: %s\n", cmd->name, cmd->doc);
    }

    return 0;
}

shell_cmd("help", help, "Display this text");
