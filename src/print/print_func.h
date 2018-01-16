#ifndef PRINT_FUNC_H
# define PRINT_FUNC_H

# include <stddef.h>
# include <stdint.h>
# include "dproc.h"

typedef void (*print_func_t)(struct dproc *proc, char *, size_t, uintptr_t);

struct print_func {
    const char format;
    print_func_t func;
};


int is_valid_format(char *format);

print_func_t get_print_func(char format);

/**
** \param proc Process to read
** \param str String to print
** \param len Length of \p str
** \param addr Starting address to print
**
** \brief Print the disassembly of \p str in hex format.
*/
void hexa_print(struct dproc *proc, char *str, size_t len, uintptr_t addr);

/**
** \param proc Process to read
** \param str String to print
** \param len Length of \p str
** \param addr Starting address to print
**
** \brief Print the disassembly of \p str four bytes by four bytes
** in decimal format.
*/
void decimal_print(struct dproc *proc, char *str, size_t len, uintptr_t addr);

/**
** \param proc Process to read
** \param str String to print
** \param len Length of \p str
** \param addr Starting address to print
** \brief Prints every null terminated string found in \p str.
*/
void string_print(struct dproc *proc, char *str, size_t len, uintptr_t addr);

/**
** \param proc Process to read
** \param str String to print
** \param len Length of \p str
** \param addr Starting address to print
**
** \brief Prints every instruction found in \p str.
*/
void instr_print(struct dproc *proc, char *str, size_t len, uintptr_t addr);

#endif /* !PRINT_FUNC_H */
