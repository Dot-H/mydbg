#ifndef BREAKPOINT_H
# define BREAKPOINT_H

#define BP_HTABLE_SIZE 128
#define BP_OPCODE (0xcc)

enum bp_type {
    BP_CLASSIC,
    BP_TEMPORARY,
    BP_HARDWARE,
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
** \brief Free all the allocated memory inside \p bp and \p bp
** itself.
*/
void bp_destroy(struct breakpoint *bp);

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
