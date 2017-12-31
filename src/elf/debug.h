#ifndef DEBUG_H
# define DEBUG_H

# include <elf.h>
# include <stddef.h>
# include <stdint.h>

# include "hash_table.h"

# define DW_HTABLE_SIZE 5

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
    char *filename;
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

ssize_t get_line_from_addr(struct htable *dw_table, uintptr_t addr);

/****************************************/
/*      Wrappers to struct htable       */
/****************************************/

/**
** \brief Call the htable_creat function with dw_hash, dw_cmp and
** DW_HTABLE_SIZE.
*/
struct htable *dw_htable_creat(void);

/**
** \brief Free all the allocated memory inside \p htable and \p htable
** itself.
*/
void dw_htable_destroy(struct htable *htable);

/**
** \brief get the struct dw_file corresponding to \p name using its hash.
**
** \return Return the struct if found and NULL otherwise
*/
struct dw_file *dw_htable_get(char *name, struct htable *htable);

/**
** \brief search in \p htable the struct dw_file where \p addr is greater
** or equal to the start attribute and lower or equal to the end attribute
**
** \return Return the found struct dw_file if any and NULL otherwise.
*/
struct dw_file *dw_htable_search_by_addr(uintptr_t addr,
                                         struct htable *htable);

/*
** \note Print an error on stderr if a file with the same name is already
** present in the table
*/
void dw_htable_insert(struct dw_file *dw, struct htable *htable);

#endif /* !DEBUG_H */
