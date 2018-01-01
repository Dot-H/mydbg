#include "include.h"

int main(void)
{
    while (1) {
        printf("main: %lx\n", (uintptr_t)main);
        sleep(a(0));
    }

    return 0;
}
