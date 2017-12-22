#include <stdio.h>
#include <string.h>

#include "commands.h"
#include "my_dbg.h"

static int unique_doc(const char *str)
{
    size_t len = __stop_cmds - __start_cmds;

    for (size_t i = 0; i < len; ++i) {
        struct command *cmd = __start_cmds + i;
        if (!strcmp(cmd->name, str)) {
            printf("%s: %s\n", cmd->name, cmd->doc);
            return 0;
        }
    }

    fprintf(stderr, "Command not found\n");
    return -1;
}

int print_help(struct debug_infos *dinfos, char *args[])
{
    (void)dinfos;
    int argc = check_params(args, 1, 2);
    if (argc == -1)
        return -1;

    if (argc == 2)
       return unique_doc(args[1]);

    size_t len = __stop_cmds - __start_cmds;

    for (size_t i = 0; i < len; ++i) {
        struct command *cmd = __start_cmds + i;
        printf(KBLU"%s"KNRM": %s\n", cmd->name, cmd->doc);
    }

    return 0;
}

shell_cmd(help, print_help, "Display this text");
