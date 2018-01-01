#include "include.h"

int c(int val)
{
    printf("c: %lx\n", (uintptr_t)c);
    fflush(stdout);
    int titi = val + 1;
    return titi;
}

