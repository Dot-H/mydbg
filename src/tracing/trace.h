#ifndef TRACE_H
# define TRACE_H

# include <sys/types.h>

struct debug_infos;
struct dproc;

void print_status(pid_t, int *status);

/**
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

/**
** \brief Wait for the tracee described by \prof to receive a signal.
** When a signaled is received, proc is filled with the signal info-
** rmations.
*/
void wait_tracee(struct debug_infos *dinfos, struct dproc *proc);

/**
** \brief Wait for the pid present in \p proc to received a SIGSTOP.
** Any other signal received before the SIGSTOP is reinjected in the
** process.
**
** \return Return -1 If the process is terminated before receiving the
** SIGSTOP or if a ptrace failed. Return 0 on success.
**
** \note A bug occures if a SIGSTOP is concurrently sent to \p proc.
** An error message is print on stderr if an error occured.
*/
int wait_attach(struct dproc *proc);

/**
** \brief Trace the pid in \p proc via PTRACE_ATTACH
**
** \return Return 0 on success and -1 if an error occured
**
** \note If an error occured, a message is print on stderr.
*/
int attach_proc(struct dproc *proc);

/**
** \brief Replace opcode at \p addr in \p pid by \opcode.
**
** \return Return the old opcode on success and -1 on failure
**
** \note If an error occurs, a message is print on stderr.
*/
long set_opcode(pid_t pid, long opcode, void *addr);

#endif /* !TRACE_H */
