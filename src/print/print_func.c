#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <capstone/capstone.h>

#include "print_func.h"

#define array_size(t) (sizeof(t) / sizeof(*t))

struct print_func print_functions[] = {
    { 'x', hexa_print },
    { 'd', decimal_print },
    { 's', string_print },
    { 'i', instr_print },
};

int is_valid_format(char *format)
{
    if (!format || !format[1] || format[2])
        return 0;

   size_t len = array_size(print_functions);
   for (size_t i = 0; i < len; ++i)
       if (format[1] == print_functions[i].format)
           return 1;

   return 0;
}

void (*get_print_func(char format))(char *, size_t, uintptr_t)
{
   size_t len = array_size(print_functions);
   for (size_t i = 0; i < len; ++i)
       if (format == print_functions[i].format)
           return print_functions[i].func;

   return NULL;
}

static inline void print_addr(uintptr_t addr)
{
    printf("0x%lx:", addr);
}

static void hexa_print_32b(const char *str, int nb)
{
    int int_size = sizeof(int);
    if (int_size > nb)
        int_size = nb;

    printf("0x");
    for (int i = int_size - 1; i >= 0; --i)
        printf("%02hhx", str[i]);
}

static void dec_print_32b(const char *str, int nb)
{
    int int_size = sizeof(int);
    if (int_size > nb)
        int_size = nb;

    int print;
    memcpy(&print, str, int_size);
    printf("%d", print);
}

static void dword_print(char *str, size_t len, uintptr_t addr,
                        void (*func)(const char *str, int nb))
{
    if (len == 0)
        return;

    size_t int_size = sizeof(int);
    size_t nb_print = len / int_size + (len % int_size != 0);

    print_addr(addr);
    putchar('\t');
    func(str, len);
    len -= int_size;

    for (size_t i = 1; i < nb_print; ++i) {
        size_t oft = i * int_size;
        if (i % int_size == 0) {
            putchar('\n');
            print_addr(addr + oft);
        }

        putchar('\t');
        func(str + oft, len);
        len -= int_size;
    }

    putchar('\n');
}

void hexa_print(char *str, size_t len, uintptr_t addr)
{
    dword_print(str, len, addr, hexa_print_32b);
}

void decimal_print(char *str, size_t len, uintptr_t addr)
{
    dword_print(str, len, addr, dec_print_32b);
}


/**
** \brief print \p str until encountering a null byte or printing
** \p len character. Non printable characters are escaped and the
** last character print is followed by a newline.
**
** \return Return the number of character print plus one for the
** nullbyte.
*/
static int print_string(const char *str, int len)
{
    int cnt = 0;
    for (; cnt < len && str[cnt]; ++cnt) {
        if (isprint(str[cnt]))
            putchar(str[cnt]);
        else
            printf("\\x%02x", str[cnt] & 0xff);
    }

    return cnt + 1;
}

void string_print(char *str, size_t len, uintptr_t addr)
{
    if (len == 0)
        return;

    int ilen = len;
    int cnt = 0;
    while (cnt < ilen)
    {
        print_addr(addr + cnt);
        printf("\t\"");
        cnt += print_string(str + cnt, len - cnt);
        printf("\"\n");
    }
}

void instr_print(char *str, size_t len, uintptr_t addr)
{
    csh handle;
    cs_insn *insn;

    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) {
        fprintf(stderr, "Failed to initialize capstone\n");
        return;
    }

    size_t count = cs_disasm(handle, (uint8_t *)str, len, addr, 0, &insn);
    if (count > 0) {
        for (size_t j = 0; j < count; j++) {
            printf("0x%lx:\t%s\t\t%s\n", insn[j].address,
                   insn[j].mnemonic, insn[j].op_str);
        }

        cs_free(insn, count);
    }
    else
        fprintf(stderr, "Failed to disassemble given code\n");


    cs_close(&handle);
}
