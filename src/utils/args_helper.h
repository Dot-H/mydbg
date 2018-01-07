#ifndef ARGS_HELPER_H
# define ARGS_HELPER_H

# include "my_dbg.h"
# include "dproc.h"

# define KNRM  "\x1B[0m"
# define KRED  "\x1B[31m"
# define KGRN  "\x1B[32m"
# define KYEL  "\x1B[33m"
# define KBLU  "\x1B[34m"
# define KMAG  "\x1B[35m"
# define KCYN  "\x1B[36m"
# define KWHT  "\x1B[37m"

/**
** \param arr Null terminated array
**
** \return Return the size of \p arr
*/
size_t nullarray_size(char *arr[]);

/**
** \param arg Null terminated string
**
** \brief convert \p arg to a long integer value according to the
** given \p base.
**
** \return Returns -1 if \p arg is not a valid integer and the
** converted integer otherwise.
**
** \note If -1 is returned, a message is print on stderr.
*/
long arg_to_long(char *arg, int base);

/**
** \args Null terminated array of string
**
** \brief Check if the number of string inside args is inside
** [\p min, \p max]. If \p max is equal to -1, max is not taken
** into account
**
** \return Return -1 if args is not valid and its size otherwise.
**
** \note Note that if the command's name is the first string of
** args, it is taken into account. If args is not valid, a message
** is print on stderr.
*/
int check_params(char *args[], long min, long max);

/**
** \return Return 1 if mydbg is tracing a process and 0 otherwise.
**
** \note If no process are traced, a message is print on stderr.
*/
int is_traced(struct debug_infos *dinfos);

/**
** \return Return 1 if the loaded elf has debugging informations and 0
** otherwise.
**
** \note If the the process has no debugging informations, an error is
** print on stderr.
*/
int has_debug_infos(struct debug_infos *dinfos);

/**
** \param idx Index of the pid
**
** \brief Get the struct dproc from either the pid given in \p args or
** the pid in \p dinfos. The pid is taken from \args if \p idx is smaller
** than \p argsc.
**
** \return Returns a struct dproc if the pid is valid and NULL otherwise.
**
** \note A message is print on stderr if the pid is invalid.
*/
struct dproc *get_proc(struct debug_infos *dinfos, char *args[], int argsc,
                       int idx);

/**
** \param idx Index of the address
**
** \brief Get the address from either \p args or RIP. The address is taken
** from \args if \p idx is smaller than \p argsc.
**
** \return Returns the address on success and -1 otherwise.
**
** \note A message is print on stderr if an error occured.
*/
long get_addr(pid_t pid, char *args[], int argsc, int idx);

/**
** \param idx Index of the pid
**
** \brief Get the pid from either \p args or \p dinfos. The pid is taken
** from \args if \p idx is smaller than \p argsc.
**
** \return Returns the pid on success and -1 otherwise.
**
** \note A message is print on stderr if an error occured.
*/
pid_t get_pid(struct debug_infos *dinfos, char *args[], int argsc, int idx);

#endif /* !ARGS_HELPER_H */
