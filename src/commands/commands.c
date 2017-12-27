#include <stddef.h>
#include <string.h>

#include "commands.h"

static inline int is_whitespace(char c)
{
    return c == '\t' || c == '\n' || c == ' ' || c == '\r';
}

char *cmd_generator(const char *text, int state)
{
    static size_t idx;
    static size_t len;
    char *name;

    /* If this is a new word to complete, initialize now.  This
     includes saving the length of TEXT for efficiency,
     initializing the index variable to 0. */
    if (!state){
        idx     = 0;
        len = strlen(text);
    }

    while ((name = __start_cmds[idx].name)){
        idx++;
        if (strncmp(name, text, len) == 0)
              return strdup(name);
    }

    return NULL;
}

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
