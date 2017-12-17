#ifndef MY_DBG_H
# define MY_DBG_H

# include <stddef.h>
# include <sys/types.h>

# include "mapping.h"
# include "hash_table.h"

# define HISTORY_FILE ".mydbg_history"
# define PROMPT "mydbg> "
# define QUIT_MSG "quit"

struct debug_infos {
    char **args;
    struct melf melf;

    pid_t  dflt_pid; /* Current default pid */
    struct htable *dproc_table; /* Hash table containing running process */
};

struct debug_infos *init_debug_infos(void);

/*
** \fn void empty_debug_infos(struct debug_infos *dinfos)
**
** \param dinfos struct debug_infos to reset.
**
** \brief free and set tu null all the allocated memory inside
** the structure. The structure itself is not free.
*/
void empty_debug_infos(struct debug_infos *dinfos);

/*
** \fn void destroy_debug_infos(struct debug_infos *dinfos)
**
** \param dinfos struct debug_infos to free.
**
** \brief free both the structure and all the allocated memory
** inside.
*/
void destroy_debug_infos(struct debug_infos *dinfos);

#endif /* !MY_DBG_H */
