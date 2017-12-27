#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

int c(int val)
{
    printf("c: %lx\n", (uintptr_t)c);
    fflush(stdout);
    int titi = val + 1;
    return titi;
}

int b(int val)
{
    printf("b: %lx\n", (uintptr_t)b);
    fflush(stdout);
    int tata = c(val) + 1;
    return tata;
}

int a(int val)
{
    printf("a: %lx\n", (uintptr_t)a);
    fflush(stdout);
    int toto = b(val) + 1;
    return toto;
}

int main(void)
{
    while (1) {
        printf("main: %lx\n", (uintptr_t)main);
        sleep(a(0));
    }

    return 0;
}
