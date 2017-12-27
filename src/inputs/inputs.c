#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "inputs.h"
#include "commands.h"
#include "my_dbg.h"

/*
** Completion code adapted from http://www.delorie.com/gnu/docs/readline/rlman_48.html
*/

static inline int is_whitespace(char c)
{
    return c == '\t' || c == '\n' || c == ' ' || c == '\r';
}

static char *get_home(const char *str)
{
    static char *home = NULL;
    static size_t len = 0;
    if (!home){
        home = getenv("HOME");
        if (!home)
            return NULL;

        len = strlen(home);
    }

    char *res = calloc(len + strlen(str) + 2, sizeof(char));
    if (!res)
    return NULL;

    strcat(res, home);
    strcat(res, "/");
    strcat(res, str);

    return res;
}

static void update_history(int ret, void *ptr_home_hist)
{
    (void)ret;
    char *home_hist = ptr_home_hist;

    if (home_hist)
        write_history(home_hist);

    free(home_hist);
}


/**
** \param text Word to complete
** \param start Start of the buffer containing the word to complete
** \param end End of the buffer containing the word to complete
**
** \brief Search all the possible matches
**
** \return Returns the array of matches or NULL if there aren't any
*/
static char **completion(const char *text, int start, int end)
{
    char **matches = NULL;
    (void)end;

    if (start == 0)
        matches = rl_completion_matches(text, cmd_generator);

    return matches;
}

void init_interaction(void)
{
    char *home_history = get_home(HISTORY_FILE);
    on_exit(update_history, home_history);

    read_history(home_history);
    rl_outstream = stderr;

    rl_attempted_completion_function = completion;
}

char *strip_whitespace(char *str)
{
    if (!str)
        return str;

    size_t idx = 0;
    while (is_whitespace(str[idx]))
        ++idx;

    if (!str[idx])
        return str + idx;

    size_t tmp = strlen(str) - 1;
    while (is_whitespace(str[tmp]))
        --tmp;

    str[tmp + 1] = '\0';
    return str + idx;
}

char **build_cmd(char *text)
{
    if (!text || !*text)
        return NULL;

    size_t cpcity = ARG_REALLOC;
    char **args   = malloc(sizeof(char *) * cpcity);
    if (!args)
        goto err_enomem_free;

    args[0] = strtok(text, WHITESPACES_DELIM);
    for (size_t i = 1; (args[i] = strtok(NULL, WHITESPACES_DELIM)); ++i)
    {
        if (i == cpcity - 1)
        {
            cpcity   += ARG_REALLOC;
            void *tmp = realloc(args, cpcity * sizeof(char *));
            if (!tmp)
                goto err_enomem_free;

            args = tmp;
        }
    }

    return args;

err_enomem_free:
    warn("Could not allocate enough memory to build arguments");
    free(args);
    return NULL;
}

char *get_line(void)
{
    char *line = readline(PROMPT);
    if (line)
        add_history(line);

    return line;
}

static char *copy_arg(char *arg)
{
    if (!arg)
        return NULL;

    size_t len = 0;
    while (arg[len] && !is_whitespace(arg[len]))
        ++len;

    char *copy = malloc(len + 1);
    strncpy(copy, arg, len);
    copy[len] = '\0';

    return copy;
}

char **dup_args(char *args[])
{
    size_t len = 0;
    for (char **tmp = args; *tmp; ++tmp)
        ++len;

    char **dup = malloc((len + 1) * sizeof(char *));
    if (!dup)
        err(1, "Failed to allocated args duplicate");

    for (size_t i = 0; i < len; ++i)
        dup[i] = copy_arg(args[i]);

    dup[len] = NULL;
    return dup;
}

void destroy_args(char **args)
{
    if (!args)
        return;

    for (char **tmp = args; *tmp; ++tmp)
        free(*tmp);

    free(args);
}
