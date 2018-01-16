#include <stdlib.h>
#include <stdio.h>

#include "commands.h"
#include "my_dbg.h"
#include "dproc.h"
#include "print_func.h"

static int do_print(char *str, struct dproc *proc, size_t len,
                    uintptr_t addr, size_t ninstr)
{
    cs_insn *insn;

    if (!proc->handle) {
        if (cs_open(CS_ARCH_X86, CS_MODE_64, &proc->handle) != CS_ERR_OK) {
            fprintf(stderr, "Failed to initialize capstone\n");
            proc->handle = 0;
            return -1;
        }
    }

    size_t count = cs_disasm(proc->handle, (uint8_t *)str, len, addr, 0, &insn);
    if (count > 0) {
        for (size_t j = 0; j < count && j < ninstr; j++) {
            printf("0x%lx:\t%s\t\t%s\n", insn[j].address,
                   insn[j].mnemonic, insn[j].op_str);
        }

        cs_free(insn, count);
    }
    else {
        fprintf(stderr, "Failed to disassemble given code\n");
        return -1;
    }


    return 0;
}

int do_disas(struct debug_infos *dinfos, char *args[])
{
    if (!is_traced(dinfos))
        return -1;

    ssize_t nb = 10;
    int argsc = check_params(args, 1, 3);
    if (argsc == -1)
        return -1;
    else if (argsc >= 2) {
        nb = arg_to_long(args[1], 10);
        if (nb < 0)
            return -1;
    }

    long addr = get_addr(dinfos->dflt_pid, args, argsc, 2);
    if (addr == -1)
        return -1;

    struct dproc *proc = get_proc(dinfos, args, argsc, 3);
    if (!proc)
        return -1;

    char *dumped = read_dproc(dinfos, proc, nb * 8, addr);
    if (!dumped)
        return -1;

    int ret = do_print(dumped, proc, nb * 8, addr, nb);
    free(dumped);

    return ret;
}

shell_cmd(disassemble, do_disas, "Disassemble N instructions");
