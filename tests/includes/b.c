#include "include.h"

int b(int val)
{
    printf("b: %lx\n", (uintptr_t)b);
    fflush(stdout);
    int tata = c(val) + 1;
    return tata;
}
