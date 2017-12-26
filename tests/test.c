#include <stdio.h>
#include <unistd.h>

void print_args(int argc, char *argv[])
{
    for (int i = 0; i < argc; ++i)
        printf("%s\n", argv[i]);
}

int main(int argc, char *argv[])
{
    if (argc > 1)
        print_args(argc, argv);
    while (1)
        sleep(10);

    return 0;
}
