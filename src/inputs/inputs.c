#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "inputs.h"
#include "commands.h"
#include "my_dbg.h"

/*
** Completion code adapted from http://www.delorie.com/gnu/docs/readline/rlman_48.html
*/

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
** \fn char **completion(const char *text, size_t start, size_t end)
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
    (void)end;
    char **matches = NULL;
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

#if 0
static inline int is_whitespace(char c)
{
    return c == '\t' || c == '\n' || c == ' ' || c == '\r';
}

static char *strip_whitespace(char *str)  
{
    if (!str)
        return str;

    size_t idx = 0;
    while (is_whitespace(str[idx]))
        ++idx;

    if (!str[idx])
        return str;

    size_t tmp = strlen(str) - 1;
    while (is_whitespace(str[tmp]))
        --tmp;

    str[tmp] = '\0';
    return str + idx;
}
#endif

char *get_line(void)
{
    char *line = readline(PROMPT);
    if (line)
        add_history(line);

    return line;
}
