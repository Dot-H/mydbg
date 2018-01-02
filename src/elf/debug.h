#ifndef DEBUG_H
# define DEBUG_H

# include <elf.h>
# include <stddef.h>
# include <stdint.h>

# include "hash_table.h"

# define DW_HTABLE_SIZE 5
# define PROL_COMP 10 // length + version + prologue_len ?

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
    int dir_idx; /* Index in directory table. 0 If any */
    char *mfile; /* Mapped file. Null if not mapped */
    size_t msize; /* size of the file */

    const struct dw_hdrline *hdr;
    uintptr_t start;
    uintptr_t end;
};

/**
** \return a newly allocated and filled with buggy values struct dw_file
*/
struct dw_file *dw_file_creat(void);

/**
** \brief Destroy \p dw and close its file descriptor if open
*/
void dw_file_destroy(struct dw_file *dw);

/**
** \brief Create an htable filled with every file's line number statement
** turned into a struct dw_file.
**
** \return Return a new allocated and filled struct htable if there is
** debug informations and NULL otherwise.
**
** \note Should change with the use of .debug_abbrev
*/
struct htable *parse_debug_info(const void *elf, const struct dwarf *dwarf);

/**
** \param dw_table Hash table in wich \p filename is supposed to be
** \param dw Struct in which lineno is searched. Can be null
** \param filename File referred by \p lineno. Ignored if \p dw is not null
** \param lineno Researched line
**
** \brief Get the struct dw_file referring to \p filename and search
** the address of \p lineno. An error occurs if \p filename or \lineno
** is not found.
**
** \return Return the address of \p lineno on success and -1 otherwise.
**
** \note \p dw is used when the calling function wants to search in a specific
** struct dw_file. If \p dw AND \p filename are null the function will segv.
*/
intptr_t get_line_addr(struct htable *dw_table, struct dw_file *dw,
                       char *filename, size_t lineno);

/**
** \param dw Struct used to store the result found informations.
**
** \brief Search the line inside the source file corresponding to \p addr.
** If found, set \p dw with the struct dw_file corresponding to the line.
**
** \return Return the line number if found and -1 otherwise.
*/
ssize_t get_line_from_addr(struct htable *dw_table, uintptr_t addr,
                           struct dw_file **dw);

/**
** \param dw Struct used to store the result found informations.
**
** \brief Search the next line inside the source file corresponding to
** \p addr.
**
** \return Return the address of the found line on success and -1 if
** \p addr does not correspond to any file.
*/
intptr_t get_next_line_addr(struct htable *dw_table, uintptr_t addr,
                            struct dw_file **dw);

/**
** \brief map the \p dw's filename attribute, using the dir_idx attribute
** if different from 0. The \p dw's mfile attribute is set with the value of
** the mapping on success.
**
** \return Return the mapped file on success and NULL otherwise.
**
** \note In case of error, a message is print on stderr.
*/
char *dw_map(struct dw_file *dw);

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
