#ifndef COMMANDS_H
# define COMMANDS_H

# include <stddef.h>

# include "args_helper.h"

struct debug_infos;

typedef int (*cmd_func)(struct debug_infos *, char *args[]);

struct command {
    char *name;
    char *doc;
    cmd_func func;
}__attribute__ ((aligned(32)));


#define shell_cmd(nam, fn, dc) \
    static struct command __cmd_ ## nam \
    __attribute__ ((section("cmds"), used)) = \
    { .name = #nam, .func = fn, .doc = dc }

extern struct command __start_cmds[];
extern struct command __stop_cmds[];

/**
** \fn struct command *find_command(const char *name)
**
** \param name Function's name researched
**
** \brief Search through the commands array a function
** whose name matches the name argument.
**
** \return Returns the command struct corresponding to the
** found function if any and NULL if no function matches
** the name given in argument.
*/
struct command *find_command(const char *name);

/**
** \fn char *command_generator(const char *text, int state)
**
** \param text Word to complete
** \param state Lets us know wether to start from scratch
**
** \brief Get the next name partially matches from the
** commands' array. Store the next index in a static variable.
** The index is set to 0 when state is at 0.
**
** \return Return the newly allocated name. If no name matches,
** return NULL.
*/
char *cmd_generator(const char *text, int state);

int load_file(struct debug_infos *dinfos, char *args[]);

int do_singlestep(struct debug_infos *dinfos, char *args[]);

int do_continue(struct debug_infos *dinfos, char *args[]);

#endif /* !COMMANDS_H */
