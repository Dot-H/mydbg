#ifndef COMMANDS_H
# define COMMANDS_H

# include <stddef.h>

struct debug_infos;

typedef int (*cmd_func)(struct debug_infos *, const char *args[]);

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

int print_help(struct debug_infos *dinfos, const char *args[]);

int do_quit(struct debug_infos *dinfos, const char *args[]);

#endif /* !COMMANDS_H */
