#include <stdio.h>
#include <sys/wait.h>

#include "commands.h"
#include "my_dbg.h"
#include "dproc.h"
#include "breakpoint.h"

const char *state_name(enum bp_state state)
{
    switch (state) {
        case BP_ENABLED:
            return "ENA";
        case BP_HIT:
            return "HIT";
        case BP_DISABLED:
            return "DIS";
    }

    return "???";
}

const char *type_name(enum bp_type type)
{
    switch (type) {
        case BP_CLASSIC:
            return "breakpoint";
        case BP_TEMPORARY:
            return "temporary breakpoint";
        case BP_HARDWARE:
            return "hardware breakpoint";
        case BP_SYSCALL:
            return "syscall breakpoint";
        default:
            return "";
    }

    return "???";
}

int print_bps(struct debug_infos *dinfos, char *args[])
{
    if (dinfos->dproc_table->nmemb == 0){
        printf("No running process\n");
        return -1;
    }

    if (check_params(args, 1, 1) == -1)
        return -1;

    struct htable *htable = dinfos->bp_table;
    printf("%zu breakpoints are put\n", htable->nmemb);

    for (size_t i = 0, j = 0; i < htable->size && j < htable->nmemb; ++i)
    {
        struct wl_list *head = &htable->array[i].link;
        struct data *tmp;

        wl_list_for_each(tmp, head, link){
            struct breakpoint *bp = tmp->value;
            if (bp->type == BP_SILENT)
                continue;

            uintptr_t bp_addr = (uintptr_t)bp->addr;
            if (bp->type == BP_HARDWARE)
                ++bp_addr;

            const char *state = state_name(bp->state);
            const char *name  = type_name(bp->type);
            const char *place = bp->type == BP_SYSCALL ? "on syscall" : "at";
            const char *dr    = bp->type == BP_HARDWARE ? "dr" : "";
            printf("\t%s %s%u [%s] %s 0x%lx hit %zu times\n",
                    name, dr, bp->id, state, place, bp_addr, bp->count);
            ++j;
        }
    }

    return 0;
}

shell_cmd(break_list, print_bps, "Print all the breakpoints");
