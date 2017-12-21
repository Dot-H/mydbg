#ifndef BREAKPOINT_H
# define BREAKPOINT_H

# include <stddef.h>
# include <sys/types.h>

# include "dproc.h"
# include "hash_table.h"
# include "my_dbg.h"

#define TRAP_BRKPT 1
#define BP_HTABLE_SIZE 128
#define BP_OPCODE 0xcc

/**
** \param BP_RESET used to a breakpoint previously hit. When a BP_RESET
** is hit, it puts a BP_OPCODE on its addr - 1 and destroys itself.
*/
enum bp_type {
    BP_CLASSIC = 0,
    BP_TEMPORARY = 1,
    BP_RESET = 2,
};

struct breakpoint {
    enum bp_type type;
    unsigned id; /* A bp has no id while it is not in the bp table */
    pid_t a_pid; /* Associated pid */
    int is_enabled;

    void *addr; /* Address of the replaced instruction / breakpoint */
    long sv_instr; /* Saved instruction */
    size_t count; /* Nb of time breakpoint has been hit */
};

/**
** \brief Allocate a new struct breakpoint filled with \p pid and \p bp
**
** \return Return the newly allocated breakpoint
*/
struct breakpoint *bp_creat(enum bp_type);

/**
** \brief Free all the allocated memory inside \p htable and reset
**  all its attributes.
*/
void bp_htable_reset(struct htable *htable);

/**
** \brief Reset the opcode at the address of \p bp before
** freeing all the allocated memory inside \p bp and \p bp
** itself.
**
** \return Returns -1 if the function failed to reset the opcode
** at the address of \p bp and 0 otherwise.
**
** \note In case of error, a message is print on stderr and
** \p bp has not been destroyed.
*/
int bp_destroy(struct breakpoint *bp);

/**
** \param dinfos Envirnment containing the breakpoint table.
** \param proc Process whose received the SIGTRAP
**
** \brief Search for the breakpoint which could haved caused the
** SIGTRAP received by \p proc. If found, the saved instruction
** is execute and the breakpoint is actualized.
**
** \return Returns 0 if a breakpoint has been found and everything
** went fine. Otherwise -1 is returned.
**
** \note If a breakpoint is found but the routine failed, an error
** is print on stderr and -1 is returned.
*/
int bp_hit(struct debug_infos *dinfos, struct dproc *proc);

/**
** \brief Creates a reset breakpoint from \p bp and insert it both
** in the process \proc and in \p htable.
**
** \return return a positive value on success and -1 on error.
**
** \note In case of error, a message is print on stderr.
*/
int bp_create_reset(struct htable *htable, struct breakpoint *bp);

/****************************************/
/*      Wrappers to struct htable       */
/****************************************/

/**
** \brief Call the htable_creat function with pid_hash, pid_cmd and
** DPROC_HTABLE_SIZE.
*/
struct htable *bp_htable_creat(void);

/**
** \brief Free all the allocated memory inside \p htable and \p htable
** itself.
*/
void bp_htable_destroy(struct htable *htable);

struct breakpoint *bp_htable_get(void *addr, struct htable *htable);

void bp_htable_remove(struct breakpoint *bp, struct htable *htable);

int bp_htable_insert(struct breakpoint *bp, struct htable *htable);

#endif /* !BREAKPOINT_H */
