#include "include.h"

int a(int val)
{
    printf("a: %lx\n", (uintptr_t)a);
    if (val == 3)
        return 0;
    fflush(stdout);
    int toto = b(val) + 1;
    return toto;
}
