#ifndef TRACE_H
# define TRACE_H

# include <sys/types.h>

struct debug_infos;

void print_status(pid_t, int *status);

int trace_binary(struct debug_infos *dinfos, int *status);

#endif /* !TRACE_H */
