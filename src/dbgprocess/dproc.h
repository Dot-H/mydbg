#ifndef DPROC_H
# define DPROC_H

# include <libunwind.h>
# include <libunwind-ptrace.h>
# include <stddef.h>
# include <signal.h>
# include <sys/types.h>
# include <sys/wait.h>

# include "hash_table.h"
# include "my_dbg.h"

# define DPROC_HTABLE_SIZE 1
# define BP_TABLEN 4

struct unwind {
    struct UPT_info *ui;
    unw_cursor_t c;
    unw_accessors_t ap;
    unw_addr_space_t as;
};

/**
** bp_hwtab holds 0 if the corresponding %drN is unset and the address inside
** the register otherwise
*/
struct dproc {
    pid_t pid;
    uintptr_t bp_hwtab[BP_TABLEN];
    int status;
    int is_attached;
    siginfo_t siginfo; /* Last signal received by \p pid */
    struct unwind unw;
};

/**
** \brief Allocate a new struct dproc filled with \p pid and \p proc
**
** \return Return the newly allocated dproc
*/
struct dproc *dproc_creat(void);

/**
** \brief Free all the allocated memory inside \p proc and \p proc
** itself.
*/
void dproc_destroy(struct dproc *proc);

/**
** \brief Free all the allocated memory inside \p htable and reset
**  all its attributes.
*/
void dproc_htable_reset(struct htable *htable);

/*
** \return Returns a positive value if the process is finished
** either by a normal exit or by a signal and 0 if the process
** is still running.
*/
int is_finished(struct dproc *proc);

/**
** \brief Read \p size bytes at \p start_addr in \proc
**
** \return Returns an allocated null-terminated string filled with the
** read bytes.
*/
char *read_dproc(struct debug_infos *dinfos, struct dproc *proc,
                 size_t size, uintptr_t start_addr);

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
