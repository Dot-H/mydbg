#include <stdio.h>

#include "commands.h"
#include "maps.h"
#include "my_dbg.h"

int do_info_memory(struct debug_infos *dinfos, char *args[])
{
    int argsc = check_params(args, 1, 2);
    if (argsc == -1 || !is_traced(dinfos))
        return -1;

    pid_t pid = get_pid(dinfos, args, argsc, 1);
    if (pid == -1)
        return -1;

    print_maps(pid);

    return 0;
}

shell_cmd(info_memory, do_info_memory, "Print /proc/[PID]/maps");
