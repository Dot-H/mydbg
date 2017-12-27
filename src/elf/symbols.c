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

    if (melf->staticsym)
        sym_htable_destroy(melf->staticsym);

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

  while ((ch = *name++) != '\0')
    h = (h << 5) + h + ch;
  return h & 0xffffffff;
}

static int sym_cmp(void *keya, void *keyb)
{
    char *stra = keya;
    char *strb = keyb;
    return !strcmp(stra, strb);
}

static inline void *add_oft(void *addr, uint64_t oft)
{
    char *ret = addr;
    return ret + oft;
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

static const Elf64_Sym *gnu_lookup(const char *strtab, const Elf64_Sym *symtab,
                      const struct gnu_table *hashtab, const char *name) {

    const uint64_t *bloom = &hashtab->bloom;
    const uint32_t* buckets = (void *)(bloom + hashtab->bloom_size);
    const uint32_t* chain = buckets + hashtab->nbuckets;
    uint32_t nb_bits = sizeof(uint64_t) * 8;

    uint32_t namehash = gnu_hash(name);
    uint64_t word = bloom[(namehash / nb_bits) % (hashtab->bloom_size)];
    uint64_t mask = 0
        | ((uint64_t)1 << (namehash % nb_bits))
        | ((uint64_t)1 << ((namehash >> hashtab->bloom_shift) % nb_bits));
    word |= mask;

    if ((word & mask) != mask) {
        return NULL;
    }

    uint32_t n = buckets[namehash % hashtab->nbuckets];
    if (n == 0)
        return NULL;

    const Elf64_Sym *sym = symtab + n;
    const uint32_t *hashval = &chain[n - hashtab->symoffset];

    for (namehash &= ~1; 1; sym++) {
        const uint32_t hash = *hashval++;
        const char* symname = strtab + sym->st_name;

        if ((namehash == (hash & ~1)) && !strcmp(name, symname))
            return sym;

        if (hash & 1)
            break;
    }

    return NULL;
}

static int get_dyn_infos(Elf64_Ehdr *header, Elf64_Sym **dynsymtab,
                         char **dynstrtab, struct gnu_table **gnutable)
{
    if (!header->e_phoff || !header->e_phnum)
        return -1;


    uint16_t phnum;
    get_ph_infos(header, &phnum);

    Elf64_Phdr *phdr = add_oft(header, header->e_phoff);
    uint16_t i = 0;
    for (; i < phnum; ++i, ++phdr)
        if (phdr->p_type == PT_DYNAMIC)
            break;

    if (i == phnum || phdr->p_type != PT_DYNAMIC)
        return -1;

    *dynsymtab = NULL;
    *dynstrtab = NULL;
    *gnutable  = NULL;
    Elf64_Dyn *dyn = add_oft(header, phdr->p_offset);
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
                            size_t *symtab_size, char **strtab)
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
            default:
                break;
        }
    }

    if (!strtab || !symtab)
        return -1;

    return 0;
}

void get_symbols(struct melf *elf)
{
    elf-> strtab    = NULL; /* Has to be NULL if no symbols */
    elf->staticsym = NULL; /* Same */
    if (get_dyn_infos(elf->elf, &elf->dynsymtab, &elf->dynstrtab,
                      &elf->gnutable) == -1) {
        fprintf(stderr, KRED"No exported symbols found\n"KNRM);
        return;
    }
    fprintf(stderr, KBLU"Exported symbols found\n"KNRM);

    size_t symtab_size = 0;
    Elf64_Sym *symtab  = NULL;
    if (get_static_infos(elf->elf, &symtab, &symtab_size, &elf->strtab) == -1) {
        fprintf(stderr, KRED"No static symbols found\n"KNRM);
        return;
    }
    fprintf(stderr, KBLU"Static symbols found\n"KNRM);

    elf->staticsym = sym_htable_creat();
    for (size_t i = 0; i < symtab_size; ++i) {
        if (ELF64_ST_TYPE(symtab[i].st_info) != STT_FUNC)
            continue;

        const char *name = symtab[i].st_name + elf->strtab;
        if (!gnu_lookup(elf->dynstrtab, elf->dynsymtab, elf->gnutable, name))
            sym_htable_insert(symtab + i, elf->strtab, elf->staticsym);
    }
}

const Elf64_Sym *find_symbol(struct melf elf, char *name)
{
    if (!elf.gnutable) {
        fprintf(stderr, "No symbol loaded\n");
        return NULL;
    }

    const Elf64_Sym *ret = gnu_lookup(elf.dynstrtab, elf.dynsymtab,
                                      elf.gnutable, name);
    if (!ret && elf.staticsym)
        ret = sym_htable_get(name, elf.staticsym);

    return ret;
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
