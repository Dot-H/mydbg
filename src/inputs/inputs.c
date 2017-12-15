#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "inputs.h"
#include "my_dbg.h"

static char *get_home(const char *str)
{
  const char *home = getenv("HOME");
  if (!home)
    return NULL;

  char *res = calloc(strlen(home) + strlen(str) + 2, sizeof(char));
  if (!res)
    return NULL;

  strcat(res, home);
  strcat(res, "/");
  strcat(res, str);

  return res;
}

void update_history(int ret, void *ptr_home_hist)
{
    ret = ret;
    char *home_hist = ptr_home_hist;

    if (home_hist)
        write_history(home_hist);

    free(home_hist);
}


void init_interaction(void)
{
    char *home_history = get_home(HISTORY_FILE);
    on_exit(update_history, home_history);

    read_history(home_history);
    rl_outstream = stderr;
}

char *get_line(void)
{
    char *line = readline(PROMPT);
    if (line)
        add_history(line);

    return line;
}
