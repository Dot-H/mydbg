#include <stdio.h>
#include <sys/wait.h>

#include "commands.h"
#include "my_dbg.h"
#include "trace.h"
#include "dproc.h"

int print_process(struct debug_infos *dinfos, const char *args[])
{
    (void)args;
    if (dinfos->dproc_table->nmemb == 0){
        printf("No running process");
        return -1;
    }


    struct htable *htable = dinfos->dproc_table;
    const char *grammar = htable->nmemb > 1 ? "are" : "is";
    printf("%zu process %s running\n", htable->nmemb, grammar);

    for (size_t i = 0, j = 0; i < htable->size && j < htable->nmemb; ++i)
    {
        struct wl_list *head = &htable->array[i].link;
        struct data *tmp;

        wl_list_for_each(tmp, head, link){
            struct dproc *proc = tmp->value; 
            printf("\t[process %d] status: %d\n", proc->pid, proc->status); 
            ++j;
        }
    }

    return 0;
}

shell_cmd(infoprocess, print_process, "Prints all running process");
