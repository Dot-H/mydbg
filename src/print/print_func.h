#ifndef PRINT_FUNC_H
# define PRINT_FUNC_H

# include <stddef.h>
# include <stdint.h>

struct print_func {
    const char format;
    void (*func)(char *, size_t, uintptr_t);
};

int is_valid_format(char *format);

void (*get_print_func(char format))(char *, size_t, uintptr_t);

void hexa_print(char *, size_t, uintptr_t);

void decimal_print(char *, size_t, uintptr_t);

void string_print(char *, size_t, uintptr_t);

void instr_print(char *, size_t, uintptr_t);

#endif /* !PRINT_FUNC_H */
