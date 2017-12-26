#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#include "commands.h"
#include "my_dbg.h"
#include "breakpoint.h"
#include "mapping.h"

int do_breakf(struct debug_infos *dinfos, char *args[])
{
//    if (!is_running(dinfos))
  //      return -1;

    int argsc = check_params(args, 2, 2);
    if (argsc == -1)
        return -1;

    const Elf64_Sym *symbol = find_symbol(dinfos->melf.elf, args[1]);
    if (!symbol)
        return -1;

    if (symbol) {
        printf("FOUND\n");
        printf("0x%lx\n", symbol->st_value);
    }
    else
        printf("NOT FOUND\n");

    return 0;
}

shell_cmd(breakf, do_breakf, "Put a breakpoint on the function given in argument");
