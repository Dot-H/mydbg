#include <stdlib.h>
#include <err.h>
#include "errno.h"
#include <stdio.h>

#include "args_helper.h"

size_t nullarray_size(char *arr[])
{
    if (!arr)
        return 0;

    size_t size = 0;
    while (arr[size])
        ++size;

    return size;
}

long arg_to_long(char *arg, int base)
{
    char *endptr = NULL;
    long res = strtol(arg, &endptr, base);
    if (*endptr || errno == ERANGE) {
        fprintf(stderr, "%s: Invalid argument\n", arg);
        return -1;
    }

    return res;
}

int check_params(char *args[], size_t min, size_t max)
{
    size_t size = nullarray_size(args);
    if (size > max) {
        fprintf(stderr, "Too many arguments\n");
        return -1;
    } else if (size < min) {
        fprintf(stderr, "Not enough argument\n");
        return -1;
    }

    return size;
}

int is_running(struct debug_infos *dinfos)
{
    if (!dinfos->melf.elf || !dinfos->dflt_pid) {
        fprintf(stderr, "No running process\n");
        return 0;
    }

    return 1;
}
