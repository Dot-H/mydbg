#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

int c(int val)
{
    int titi = val + 1;
    printf("c: %lx\n", (uintptr_t)c);
    fflush(stdout);
    return titi;
}

int b(int val)
{
    int tata = c(val) + 1;
    printf("b: %lx\n", (uintptr_t)b);
    fflush(stdout);
    return tata;
}

int a(int val)
{
    int toto = b(val) + 1;
    printf("a: %lx\n", (uintptr_t)a);
    fflush(stdout);
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
