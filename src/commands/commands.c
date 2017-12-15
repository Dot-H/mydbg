#include <stddef.h>
#include <string.h>

#include "commands.h"

struct command *find_command(const char *name)
{
    size_t len = __stop_cmds - __start_cmds;

    for (size_t i = 0; i < len; ++i) {
        struct command *cmd = __start_cmds + i;
        if (!strcmp(name, cmd->name)) {
            return cmd;
        }
    }

    return NULL;
}
