#ifndef DEBUG_H
# define DEBUG_H

# include <elf.h>
# include <stddef.h>
# include <stdint.h>

# include "hash_table.h"

struct dw_hdrline {
    uint32_t length;
    uint16_t version;
    uint32_t prologue_len;
    uint8_t   min_instr_len;
    uint8_t   init_val_is_stmt;
    int8_t   line_base;
    uint8_t   line_range;
    uint8_t   opcode_base;
    uint8_t std_opcodes[12];
}__attribute__((packed));

struct dwarf {
    void *debug_info;
    Elf64_Shdr *debug_line;
};

struct dw_file {
    const char *filename;
    const struct dw_hdrline *hdr;
    uintptr_t start;
    uintptr_t end;
};

/**
** \brief Create an htable filled with every file's line number statement
** turned into a struct dw_file.
**
** \return Return a new allocated and filled struct htable if there is
** debug informations and NULL otherwise.
*/
struct htable *parse_debug_info(const void *elf, const struct dwarf *dwarf);

#endif /* !DEBUG_H */
