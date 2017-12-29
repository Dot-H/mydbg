#ifndef BREAKPOINT_H
# define BREAKPOINT_H

# include <stddef.h>
# include <sys/types.h>

# include "dproc.h"
# include "hash_table.h"
# include "my_dbg.h"

#define TRAP_BRKPT 1
#define BP_HTABLE_SIZE 10
#define BP_OPCODE 0xcc

/* bp_type's value gives the index in the bp_handlers (bp_hit.c) */
enum bp_type {
    BP_CLASSIC   = 0,
    BP_TEMPORARY = 1,
    BP_SYSCALL   = 2,
};

enum bp_state {
    BP_ENABLED,
    BP_DISABLED,
    BP_HIT, // Currently hit
};

/*
** The addr attribute of a BP_SYSCALL is its syscall number and the
** sv_instr attribute is ignored.
*/
struct breakpoint {
    enum bp_type type;
    unsigned id; /* A bp has no id while it is not in the bp table */
    pid_t a_pid; /* Associated pid */
    enum bp_state state;

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

/*
** \brief Fill \p bp with \p bp_addr, \p pid and BP_ENABLED
** before setting the BP_OPCODE in the process \p pid. Also
** insert the breakpoint in the bp_table from \p dinfos. If
** something goes wrong, \p bp is free.
**
** \return Return 0 if everything went fine and -1 on error.
**
** \note An error message is print on stderr in case of error.
*/
int bp_set(struct debug_infos *dinfos, struct breakpoint *bp,
           void *bp_addr, pid_t pid);

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
** is execute and the breakpoint is actualized. Moreover, a message
** is print on stdout to tell which breakpoint has been hit.
**
** \return Returns 0 if a breakpoint has been found and everything
** went fine. Otherwise -1 is returned.
**
** \note If a breakpoint is found but the routine failed, an error
** is print on stderr and -1 is returned.
*/
int bp_hit(struct debug_infos *dinfos, struct dproc *proc);

/**
** \brief Search for a breakpoint corresponding to the syscall value.
** If the hit breakpoint does not correspond to a return from a
** syscall, update its count and save \p bp in bp_out_sys. Otherwise,
** call out_syscall.
**
** \return Return 0 on success and -1 if no breakpoint is put on the
** called syscall or if an error occured.
**
** \note In cause of error, a message is print on stderr.
*/
int bp_sys_hit(struct debug_infos *dinfos, struct dproc *proc);

/**
** \param dinfos Envirnment containing the breakpoint table.
** \param proc Process whose received the SIGTRAP
** \param bp breakpoint whose provocted the SIGTRAP
**
** \brief Singlestep the current instruction and replace the
** breakpoint.
**
** \return Return 0 if everything went fine and -1 otherwise.
*/

int bp_reset(struct debug_infos *dinfos, struct breakpoint *bp,
             struct dproc *proc);

/**
** \brief Test if \p proc was stopped by a breakpoint and if it
** was, reset the breakpoint with bp_reset. Then set the ptrace
** request if presence of BP_SYSCALL or not.
**
** \return Return -1 if something went wrong while reseting a
** breakpoint and 0 otherwise.
**
** \note This function must be call before every restart of a tracee
** stopped by a SIGSTOP.
*/
int bp_cont(struct debug_infos *dinfos, struct dproc *proc);

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

/**
** \brief Search through the htable a breakpoint with an id equals to
** \p id. The breakpoint, if found, is destroyed and removed from
** \p htable.
**
** \return Return -1 if \p id does not correspond to an existing breakpoint
** and 0 if the corresponding breakpoint has been found and removed.
*/
int bp_htable_remove_by_id(long id, struct htable *htable);

int bp_htable_insert(struct breakpoint *bp, struct htable *htable);

#endif /* !BREAKPOINT_H */
