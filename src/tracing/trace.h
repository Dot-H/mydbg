#ifndef TRACE_H
# define TRACE_H

# include <sys/types.h>

struct debug_infos;
struct dproc;

void print_status(pid_t, int *status);

/*
** \brief Run the executable in \p dinfos->args, trace it and
** wait for it to be attached. Once all of it is done, fills
** \p proc with the siginfo, the pid of the newly created process
** and a newly create UPT.
**
** \return Returns the pid on sucess and -1 on failure
**
** \note An error message is printed on stderr in case of err.
*/
int trace_binary(struct debug_infos *dinfos, struct dproc *proc);

/*
** \brief Wait for the tracee described by \prof to receive a signal.
** When a signaled is received, proc is filled with the signal info-
** rmations.
*/
void wait_tracee(struct debug_infos *dinfos, struct dproc *proc);

/*
** \brief Replace opcode at \p addr in \p pid by \opcode.
**
** \return Return the old opcode on success and -1 on failure
**
** \note If an error occurs, a message is print on stderr.
*/
long set_opcode(pid_t pid, long opcode, void *addr);

#endif /* !TRACE_H */
