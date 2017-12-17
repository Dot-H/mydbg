#ifndef DPROC_H
# define DPROC_H

# include <stddef.h>
# include <sys/types.h>

# include "hash_table.h"

# define DPROC_HTABLE_SIZE 10

struct dproc {
   pid_t pid;
   int status;
};

/**
** \brief Allocate a new struct dproc filled with \p pid and \p proc
**
** \return Return the newly allocated dproc
*/
struct dproc *dproc_creat(pid_t pid, int status);

/**
** \brief Free all the allocated memory inside \p htable and reset
**  all its attributes.
*/
void dproc_htable_reset(struct htable *htable);

/**
** \brief Free all the allocated memory inside \p proc and \p proc
** itself.
*/
void dproc_destroy(struct dproc *proc);

/****************************************/
/*      Wrappers to struct htable       */
/****************************************/

/**
** \brief Call the htable_creat function with pid_hash, pid_cmd and
** DPROC_HTABLE_SIZE.
*/
struct htable *dproc_htable_creat(void);

/**
** \brief Free all the allocated memory inside \p htable and \p htable
** itself.
*/
void dproc_htable_destroy(struct htable *htable);

struct dproc *dproc_htable_get(pid_t pid, struct htable *htable);

void dproc_htable_remove(struct dproc *proc, struct htable *htable);

int dproc_htable_insert(struct dproc *proc, struct htable *htable);

#endif /* !DPROC_H */
