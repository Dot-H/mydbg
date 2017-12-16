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

struct dproc *dproc_creat(pid_t pid, int status);

void dproc_destroy(struct dproc *proc);

/* Wrappers functions for struct htable */

struct htable *dproc_htable_creat(void);

void dproc_htable_destroy(struct htable *htable);

struct dproc *dproc_htable_get(pid_t pid, struct htable *htable);

void dproc_htable_remove(struct dproc *proc, struct htable *htable);

int dproc_htable_insert(struct dproc *proc, struct htable *htable);

#endif /* !DPROC_H */
