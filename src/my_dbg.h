#ifndef MY_DBG_H
# define MY_DBG_H

# include <stddef.h>

# include "mapping.h"

# define HISTORY_FILE ".mydbg_history"
# define PROMPT "mydbg> "
# define QUIT_MSG "quit"

struct debug_infos {
    struct melf melf;
};

struct debug_infos *init_debug_infos(void *elf, size_t size);

int destroy_debug_infos(struct debug_infos *dinfos);

#endif /* !MY_DBG_H */
