#include <stdio.h>
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

static void hexa_print_32(char *str, int nb)
{
    int int_size = sizeof(int);
    if (int_size > nb)
        int_size = nb;

    printf("0x");
    for (int i = int_size - 1; i >= 0; --i)
        printf("%02hhx", str[i]);
}

void hexa_print(char *str, size_t len, uintptr_t addr)
{
    if (len == 0)
        return;

    size_t int_size = sizeof(int);
    size_t nb_print = len / int_size + (len % int_size != 0);

    print_addr(addr);
    putchar('\t');
    hexa_print_32(str, len);
    len -= int_size;

    for (size_t i = 1; i < nb_print; ++i) {
        size_t oft = i * int_size;
        if (i % int_size == 0) {
            putchar('\n');
            print_addr(addr + oft);
        }

        putchar('\t');
        hexa_print_32(str + oft, len);
        len -= int_size;
    }

    putchar('\n');
}

void decimal_print(char *str, size_t len, uintptr_t addr)
{
    (void)str;
    (void)len;
    (void)addr;
}

void string_print(char *str, size_t len, uintptr_t addr)
{
    if (len == 0)
        return;

    print_addr(addr);
    int ilen = len;
    int cnt  = printf("\t\"%s\"", str);

    while (cnt < ilen)
    {
        print_addr(addr);
        cnt += printf("\t\"%s\"\n", str);
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
