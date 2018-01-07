#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <elf.h>
#include <link.h>

int c(int val)
{
    printf("c: %lx\n", (uintptr_t)c);
    printf("This is a test string\n");
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

void dump_rdebug(struct r_debug *rdbg)
{
    struct link_map *tmp = rdbg->r_map;
    printf("r_debug: %p\n", (void *)rdbg);
    printf("r_brk: %p\n", (void *)rdbg->r_brk);
    printf("%s\n", tmp->l_name);
    tmp = tmp->l_next;
    for (; tmp && tmp != rdbg->r_map; tmp = tmp->l_next)
        printf("%s\n", tmp->l_name);
}

int main(void)
{
    int count = 0;
    while (1) {
//        printf("_DYNAMIC: %p\n", (void *)_DYNAMIC);
        printf("Count: %d\n", count);
        struct r_debug *r_debug;
        for (Elf64_Dyn *dyn = _DYNAMIC; dyn->d_tag != DT_NULL; ++dyn)
            if (dyn->d_tag == DT_DEBUG)
                r_debug = (struct r_debug *) dyn->d_un.d_ptr;
        dump_rdebug(r_debug);
        printf("main: %lx\n", (uintptr_t)main);
        sleep(a(0));
        ++count;
    }

    return 0;
}
