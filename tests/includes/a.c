#include "include.h"

int a(int val)
{
    printf("a: %lx\n", (uintptr_t)a);
    fflush(stdout);
    int toto = b(val) + 1;
    return toto;
}
