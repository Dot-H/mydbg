#ifndef MAPPING_H
# define MAPPING_H

# include <elf.h>
# include <stddef.h>

# include "debug.h"

# define SYM_HTABLE_SIZE 10
# define KNRM  "\x1B[0m"
# define KRED  "\x1B[31m"
# define KBLU  "\x1B[34m"

struct gnu_table {
    uint32_t nbuckets;
    uint32_t symoffset;
    uint32_t bloom_size;
    uint32_t bloom_shift;
    uint64_t bloom;
};


struct melf {
    void  *elf;
    size_t size;

    char *strtab; /* Used to get the symbols from symtab */
    struct htable *sym_table; /* Function symbols not present in gnu_hash */
    struct htable *dw_table;

    Elf64_Rela *rela_plt;
    Elf64_Sym *dynsymtab;
    char *dynstrtab;
    struct gnu_table *gnutable;
};

/**
** \param file File to map.
** \param size Size of the returned mapping.
**
** \brief Map the file given in argument and fills the size
** argument with the size of the mapping.
**
** \return Returns a pointer to the mapped area on success
** and NULL in case of error.
*/
void *map_elf(const char *file, size_t *size);

void reset_melf(struct melf *melf);

Elf64_Phdr *get_dynamic_phdr(Elf64_Ehdr *header);

/**
** \brief Search for all the function symbols and debugging
** information present in the elf and if found, fill elf->sym_tab and
** elf->dw_table with it. If one or both of the table is not filled,
** the value is set to NULL.
**
** \note The \p elf->rela_plt is also set.
**
** \note Print on stderr if it founds or not a the phdr, shdr and
** debugging informations.
*/
void get_symbols(struct melf *elf);


/**
** \brief Get the first Elf64_Rela corresponding to \p sym from
** \p elf->rela_plt.
*/
const Elf64_Rela *get_rela(struct melf elf, const Elf64_Sym *sym);

/**
** \brief Search the first symbol matching \p name in the gnu_hash_table
** and if not found, search in the \p elf->sym_tab.
**
** \return If no symbol match \p name then NULL is returned. Otherwise
** the corresponding struct Elf64_Sym is returned.
*/
const Elf64_Sym *find_symbol(struct melf elf, char *name);

/****************************************/
/*      Wrappers to struct htable       */
/****************************************/

/**
** \brief Call the htable_creat function with gnu_hash, sym_cmp and
** SYM_HTABLE_SIZE.
*/
struct htable *sym_htable_creat(void);

/**
** \brief Free all the allocated memory inside \p htable and \p htable
** itself.
*/
void sym_htable_destroy(struct htable *htable);

Elf64_Sym *sym_htable_get(char *name, struct htable *htable);

int sym_htable_insert(Elf64_Sym *sym, char *strtab, struct htable *htable);

#endif /* !MAPPING_H */
