#include <err.h>
#include <link.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "mapping.h"
#include "hash_table.h"

void reset_melf(struct melf *melf)
{
    if (!melf->elf)
        return;

    if (melf->elf && munmap(melf->elf, melf->size) == -1)
        warn("Cannot unmap %p", melf->elf);

    if (melf->sym_table)
        sym_htable_destroy(melf->sym_table);

    memset(melf, 0, sizeof(struct melf));
}

static unsigned long gnu_hash (const char *namearg)
{
  const unsigned char *name = (const unsigned char *) namearg;
  unsigned long h = 5381;
  unsigned char ch;

  while ((ch = *name++) != '\0')
    h = (h << 5) + h + ch;
  return h & 0xffffffff;
}

static size_t sym_hash (void *namearg)
{
  const unsigned char *name = (const unsigned char *) namearg;
  unsigned long h = 5381;
  unsigned char ch;

  while ((ch = *name++) != '\0' && ch != '@')
    h = (h << 5) + h + ch;
  return h & 0xffffffff;
}

static int sym_cmp(void *keya, void *keyb)
{
    char *stra = keya;
    char *strb = keyb;
    while (*stra && *strb && *stra++ == *strb++)
        continue;

    return (!*stra && !*strb) || *stra == '@' || *strb == '@';
}

static inline void *add_oft(const void *addr, const uint64_t oft)
{
    return (char *)addr + oft;
}

static void get_sh_infos(const Elf64_Ehdr *header, const Elf64_Shdr *shdr,
                  uint16_t *shnum, uint16_t *shstrndx)
{
    if (header->e_shnum)
        *shnum = header->e_shnum;

    *shnum    = (header->e_shnum) ? header->e_shnum : shdr[0].sh_size;

    if (header->e_shstrndx == SHN_XINDEX)
        *shstrndx = shdr[0].sh_link;
    else
        *shstrndx = header->e_shstrndx;
}

static void get_ph_infos(Elf64_Ehdr *header, uint16_t *phnum)
{
    if (header->e_phnum != PN_XNUM) {
        *phnum = header->e_phnum;
    } else {
        Elf64_Shdr *shdr = add_oft(header, header->e_shoff);
        *phnum = shdr[0].sh_info;
    }

}

static const Elf64_Sym *gnu_lookup(char *strtab, const Elf64_Sym *symtab,
                      const struct gnu_table *hashtab, char *name) {

    const uint64_t *bloom = &hashtab->bloom;
    const uint32_t* buckets = (void *)(bloom + hashtab->bloom_size);
    const uint32_t* chain = buckets + hashtab->nbuckets;
    uint32_t nb_bits = sizeof(uint64_t) * 8;

    uint32_t namehash = gnu_hash(name);
    uint32_t h2 = namehash >> hashtab->bloom_shift;
    uint32_t N = ((namehash / nb_bits) % hashtab->bloom_size);

    uint64_t bitmask = ((size_t)1 << (namehash % nb_bits))
                       | ((size_t)1 << (h2 % nb_bits));

    uint64_t word = bloom[N];
    if ((word & bitmask) != bitmask)
        return NULL;

    uint32_t n = buckets[namehash % hashtab->nbuckets];
    if (n == 0)
        return NULL;

    const Elf64_Sym *sym = symtab + n;
    const uint32_t *hashval = &chain[n - hashtab->symoffset];
    for (namehash &= ~1; 1; sym++) {
        const uint32_t hash = *hashval++;
        char* symname = strtab + sym->st_name;

        if ((namehash == (hash & ~1)) && sym_cmp(name, symname))
            return sym;

        if (hash & 1)
            break;
    }

    return NULL;
}

Elf64_Phdr *get_dynamic_phdr(Elf64_Ehdr *header)
{
    if (!header->e_phoff || !header->e_phnum)
        return NULL;


    uint16_t phnum;
    get_ph_infos(header, &phnum);

    Elf64_Phdr *phdr = add_oft(header, header->e_phoff);
    uint16_t i = 0;
    for (; i < phnum; ++i, ++phdr)
        if (phdr->p_type == PT_DYNAMIC)
            break;

    if (i == phnum || phdr->p_type != PT_DYNAMIC)
        return NULL;

    return phdr;
}

static int get_dyn_infos(Elf64_Ehdr *header, Elf64_Sym **dynsymtab,
                         char **dynstrtab, struct gnu_table **gnutable)
{
    *dynsymtab = NULL;
    *dynstrtab = NULL;
    *gnutable  = NULL;
    Elf64_Phdr *dyn_phdr = get_dynamic_phdr(header);
    if (!dyn_phdr)
        return -1;
    Elf64_Dyn *dyn = add_oft(header, dyn_phdr->p_offset);

    for (; dyn->d_tag != DT_NULL; ++dyn) {
        switch (dyn->d_tag) {
            case DT_STRTAB:
                *dynstrtab = add_oft(header, dyn->d_un.d_ptr);
                break;
            case DT_SYMTAB:
                *dynsymtab = add_oft(header, dyn->d_un.d_ptr);
                break;
            case DT_GNU_HASH:
                *gnutable = add_oft(header, dyn->d_un.d_ptr);
                break;
            default:
                break;
        }
     }

    if (!dynsymtab || !dynstrtab || !gnutable)
        return -1;

    return 0;
}

static int get_static_infos(Elf64_Ehdr *header, Elf64_Sym **symtab,
                            size_t *symtab_size, char **strtab,
                            Elf64_Rela **rela_plt)
{
    if (!header->e_shoff || !header->e_shentsize ||
        header->e_shstrndx == SHN_UNDEF)
        return -1;

    uint16_t shnum;
    uint16_t shstrndx;

    Elf64_Shdr *shdr = add_oft(header, header->e_shoff);
    get_sh_infos(header, shdr, &shnum, &shstrndx);
    if (!shdr[0].sh_type == SHT_NULL)
        return -1;

    char *shstrtab = add_oft(header, shdr[shstrndx].sh_offset);

    *strtab = NULL;
    *symtab = NULL;
    *rela_plt = NULL;
    for(uint16_t i = 0; i < header->e_shnum; ++i){
        const char *name = shstrtab + shdr[i].sh_name;
        switch (shdr[i].sh_type) {
            case SHT_STRTAB:
                if (!strcmp(name, ".strtab"))
                    *strtab = add_oft(header, shdr[i].sh_offset);
                break;
            case SHT_SYMTAB:
                if (!strcmp(name, ".symtab")) {
                    *symtab      = add_oft(header, shdr[i].sh_offset);
                    *symtab_size = shdr[i].sh_size / shdr[i].sh_entsize;
                }
                break;
            case SHT_RELA:
                if (!strcmp(name, ".rela.plt"))
                    *rela_plt = add_oft(header, shdr[i].sh_offset);
            default:
                break;
        }
    }

    if (!strtab || !symtab || !rela_plt)
        return -1;

    return 0;
}

static void fill_sym_table(struct melf *elf, Elf64_Sym *symtab,
                           size_t symtab_size)
{
    Elf64_Sym *tmp = elf->dynsymtab;
    for (; !tmp->st_value; ++tmp) {
        if (ELF64_ST_TYPE(tmp->st_info) != STT_FUNC)
                continue;

        sym_htable_insert(tmp, elf->dynstrtab, elf->sym_table);
    }

    for (size_t i = 0; i < symtab_size; ++i) {
        if (!symtab[i].st_value || ELF64_ST_TYPE(symtab[i].st_info) != STT_FUNC)
            continue;

        char *name = symtab[i].st_name + elf->strtab;
        if (!gnu_lookup(elf->dynstrtab, elf->dynsymtab, elf->gnutable, name))
            sym_htable_insert(symtab + i, elf->strtab, elf->sym_table);
    }
}

void get_symbols(struct melf *elf)
{
    elf->strtab    = NULL; /* Has to be NULL if no symbols */
    elf->sym_table = NULL; /* Same */
    elf->rela_plt  = NULL;
    if (get_dyn_infos(elf->elf, &elf->dynsymtab, &elf->dynstrtab,
                      &elf->gnutable) == -1) {
        fprintf(stderr, KRED"No exported symbols found\n"KNRM);
        return;
    }
    fprintf(stderr, KBLU"Exported symbols found\n"KNRM);

    size_t symtab_size = 0;
    Elf64_Sym *symtab  = NULL;
    if (get_static_infos(elf->elf, &symtab, &symtab_size, &elf->strtab,
                         &elf->rela_plt) == -1) {
        fprintf(stderr, KRED"No static symbols found\n"KNRM);
        return;
    }
    fprintf(stderr, KBLU"Static symbols found\n"KNRM);

    elf->sym_table = sym_htable_creat();
    fill_sym_table(elf, symtab, symtab_size);
}

const Elf64_Sym *find_symbol(struct melf elf, char *name)
{
    if (!elf.gnutable) {
        fprintf(stderr, "No symbol loaded\n");
        return NULL;
    }

    const Elf64_Sym *ret = gnu_lookup(elf.dynstrtab, elf.dynsymtab,
                                      elf.gnutable, name);
    if (!ret && elf.sym_table)
        ret = sym_htable_get(name, elf.sym_table);

    return ret;
}

static inline
uint16_t idx_from_oft(const void *ptr_base, const void *ptr_oft, size_t size) {
    return (((char *)ptr_oft - (char *)ptr_base) / size);
}

const Elf64_Rela *get_rela(struct melf elf, const Elf64_Sym *sym)
{
    /*
    ** Since all the relocatable symbols have been saved from the dynamic
    ** symbol table, we are sure when getting here that the symbol is present
    ** in the .rela.plt section and the idx can be calculated from the offset
    */
    uint16_t sym_idx = idx_from_oft(elf.dynsymtab, sym, sizeof(Elf64_Sym));

    Elf64_Rela *tmp = elf.rela_plt;
    while (ELF64_R_SYM(tmp->r_info) != sym_idx) // Must be in it
        ++tmp;

    return tmp;
}

struct htable *sym_htable_creat(void)
{
   return htable_creat(sym_hash, SYM_HTABLE_SIZE, sym_cmp);
}

void sym_htable_destroy(struct htable *htable)
{
    for (size_t i = 0, j = 0; i < htable->size && j < htable->nmemb; ++i)
    {
        struct wl_list *head = &htable->array[i].link;
        struct data *pos    = wl_container_of(head->next, pos, link);
        while (&pos->link != head)
        {
            struct data *tmp = pos;
            pos = wl_container_of(pos->link.next, pos, link);

            wl_list_remove(&tmp->link);
            free(tmp);
            ++j;
        }
    }

    free(htable->array);
    free(htable);
}

Elf64_Sym *sym_htable_get(char *name, struct htable *htable)
{
    struct data *sym = htable_get(htable, name);
    if (!sym)
        return NULL;

    return sym->value;
}


int sym_htable_insert(Elf64_Sym *sym, char *strtab, struct htable *htable)
{
    return htable_insert(htable, sym, strtab + sym->st_name);
}
