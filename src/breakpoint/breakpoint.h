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
#define BP_NHW 4 /* Maximum number of hardware breakpoint */
#define BP_HW_DFLT_DR6 0xffff0ff0

#define DR_OFFSET(x) (((struct user *)0)->u_debugreg + x)

#define dr7_set_hwlabel(dr7, label, idx, val)   \
    switch (idx) {                              \
        case 0:                                 \
            (dr7)->dr0_##label = val;           \
            break;                              \
        case 1:                                 \
            (dr7)->dr1_##label = val;           \
            break;                              \
        case 2:                                 \
            (dr7)->dr2_##label = val;           \
            break;                              \
        case 3:                                 \
            (dr7)->dr3_##label = val;           \
            break;                              \
    }                                           \

#define dr_get_hwlabel(dr, label, idx, val)     \
    switch (idx) {                              \
        case 0:                                 \
            val = (dr)->dr0_##label;            \
            break;                              \
        case 1:                                 \
            val = (dr)->dr1_##label;            \
            break;                              \
        case 2:                                 \
            val = (dr)->dr2_##label;            \
            break;                              \
        case 3:                                 \
            val = (dr)->dr3_##label;            \
            break;                              \
    }                                           \

/* bp_type's value gives the index in the bp_handlers (bp_hit.c) */
enum bp_type {
    BP_CLASSIC    = 0,
    BP_TEMPORARY  = 1,
    BP_SILENT     = 2, /* Used as for intern operations (finish, line_step..) */
    BP_HARDWARE   = 3,
    BP_SYSCALL,
};

enum bp_state {
    BP_ENABLED,
    BP_DISABLED,
    BP_HIT, // Currently hit
};

/*
** The addr attribute of a BP_SYSCALL is its syscall number and the
** sv_instr attribute is ignored.
**
** The BP_HARDWARE has special values:
**      id represents its dr offset (0, 1, 2, 3)
**      addr is decremented by one to act like an execution when hit
**      sv_instr stores the current value of a watchpoint (w / rw)
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

enum bp_hw_cond {
    BP_HW_INSTR        = 0,
    BP_HW_WRONLY       = 1,
    BP_HW_RDWR         = 2, // Not used
    BP_HW_RDWR_NOFETCH = 3
};

enum bp_hw_len {
    BP_HW_LEN_1BY = 0,
    BP_HW_LEN_2BY = 1,
    BP_HW_LEN_8BY = 2,
    BP_HW_LEN_4BY = 3
};

struct dr7 {
    unsigned dr0_local:       1; // 0
    unsigned dr0_global:      1; // 1
    unsigned dr1_local:       1; // 2
    unsigned dr1_global:      1; // 3
    unsigned dr2_local:       1; // 4
    unsigned dr2_global:      1; // 5
    unsigned dr3_local:       1; // 6
    unsigned dr3_global:      1; // 7
    unsigned local_exact:     1; // 8
    unsigned global_exact:    1; // 9
    unsigned reserved_1:      3; // 10 - 11 - 12
    unsigned general_detect:  1; // 13
    unsigned reserved_null:   2; // 14 - 15
    enum bp_hw_cond dr0_cond: 2; // 16 - 17
    enum bp_hw_len  dr0_len:  2; // 18 - 19
    enum bp_hw_cond dr1_cond: 2; // 20 - 21
    enum bp_hw_len  dr1_len:  2; // 22 - 23
    enum bp_hw_cond dr2_cond: 2; // 24 - 25
    enum bp_hw_len  dr2_len:  2; // 26 - 27
    enum bp_hw_cond dr3_cond: 2; // 28 - 29
    enum bp_hw_len  dr3_len:  2; // 30 - 31
};

struct dr6 {
    unsigned dr0_detected:   1; // 0
    unsigned dr1_detected:   1; // 1
    unsigned dr2_detected:   1; // 2
    unsigned dr3_detected:   1; // 3
    unsigned long reserved0: 8; // 4 - 11]
    unsigned zero_reserved:  1; // 12
    unsigned access_dtcd:    1; // 13
    unsigned single_step:    1; // 14
    unsigned task_swaitch:   1; // 15
    unsigned long reserved1: 8; // 16 - 23]
    unsigned long reserved2: 8; // 24 - 31]
};

/**
** \brief Allocate a new struct breakpoint filled with \p pid and \p bp
**
** \return Return the newly allocated breakpoint
*/
struct breakpoint *bp_creat(enum bp_type);

/**
** \param bp Breakpoint to put
** \param cond Breakpoint condition
** \param len Specify the size of the memory location at the address in \p bp
**
** \brief Search for the first available debugg register and put \p bp
** in it.
**
** \return Return -BP_NHW if there is no available register, -1 if an error
** occured with a ptrace call and 0 if evrything went fine.
**
** \note \p bp address id decrement by one after beeing put in the debug
** register because of the rip - 1 in the function bp_hit.
**
** \note An error message is print on stderr if a ptrace call failed.
*/
int bp_hw_poke(struct debug_infos *dinfos, struct breakpoint *bp,
               enum bp_hw_cond cond, enum bp_hw_len len);

/*
** \brief Fill \p bp with \p bp_addr, \p pid and BP_ENABLED.
** If the breakpoint is not a BP_HARDWARE, also set the BP_OPCODE
** in the process \p pid. Otherwise, put the breakpoint on the
** first available debbug register. Finally insert the breakpoint
** in the bp_table from \p dinfos. If something goes wrong, \p bp
** is destroy.
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
void bp_htable_reset(struct debug_infos *dinfos, struct htable *htable);

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
int bp_destroy(struct debug_infos *dinfos, struct breakpoint *bp);

/**
** \brief set the dr_local attribute of %dr7 at offset \p dr_offset to 0
** in the process pointed by \p pid.
**
** \return Return 0 on success and -1 on failure.
**
** \note An error message is print on stderr if something went wrong
*/
int bp_hw_unset(struct debug_infos *dinfos, int dr_offset, pid_t pid);

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
void bp_htable_destroy(struct debug_infos *dinfos, struct htable *htable);

struct breakpoint *bp_htable_get(void *addr, struct htable *htable);

void bp_htable_remove(struct debug_infos *dinfos, struct breakpoint *bp,
                      struct htable *htable);

/**
** \param is_hw 1 if the id corresponds to an hardware breakpoint and 0 if not
**
** \brief Search through the htable a breakpoint with an id equals to
** \p id. The breakpoint, if found, is destroyed and removed from
** \p htable.
**
** \return Return -1 if \p id does not correspond to an existing breakpoint
** and 0 if the corresponding breakpoint has been found and removed.
*/
int bp_htable_remove_by_id(struct debug_infos *dinfos, long id, int is_hw,
                           struct htable *htable);

int bp_htable_insert(struct breakpoint *bp, struct htable *htable);

#endif /* !BREAKPOINT_H */
