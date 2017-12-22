#ifndef ARGS_HELPER_H
# define ARGS_HELPER_H

# include "my_dbg.h"

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

/*
** \args Null terminated array of string
**
** \brief Check if the number of string inside args is inside
** [\p min, \p max].
**
** \return Return -1 if args is not valid and its size otherwise.
**
** \note Note that if the command's name is the first string of
** args, it is taken into account. If args is not valid, a message
** is print on stderr.
*/
int check_params(char *args[], size_t min, size_t max);

/*
** \args dinfos debugging informations
**
** \return Returns 1 if the the process is running and 0 otherwise.
**
** \note If the process is not running, a message is print on stderr.
*/
int is_running(struct debug_infos *dinfos);

#endif /* !ARGS_HELPER_H */
