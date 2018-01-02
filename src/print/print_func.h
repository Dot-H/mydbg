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

/**
** \param str String to print
** \param len Length of \p str
** \param addr Starting address to print
**
** \brief Print the disassembly of \p str in hex format.
*/
void hexa_print(char *str, size_t len, uintptr_t addr);

/**
** \param str String to print
** \param len Length of \p str
** \param addr Starting address to print
**
** \brief Print the disassembly of \p str four bytes by four bytes
** in decimal format.
*/
void decimal_print(char *str, size_t len, uintptr_t addr);

/**
** \param str String to print
** \param len Length of \p str
** \param addr Starting address to print
** \brief Prints every null terminated string found in \p str.
*/
void string_print(char *str, size_t len, uintptr_t addr);

/**
** \param str String to print
** \param len Length of \p str
** \param addr Starting address to print
**
** \brief Prints every instruction found in \p str.
*/
void instr_print(char *str, size_t len, uintptr_t addr);

#endif /* !PRINT_FUNC_H */
