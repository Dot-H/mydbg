#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "mapping.h"

unsigned long gnu_hash (const char *namearg)
{
  const unsigned char *name = (const unsigned char *) namearg;
  unsigned long h = 5381;
  unsigned char ch;

  while ((ch = *name++) != '\0')
    h = (h << 5) + h + ch;
  return h & 0xffffffff;
}

static inline void *add_oft(void *addr, uint64_t oft)
{
    char *ret = addr;
    return ret + oft;
}

void get_sh_infos(Elf64_Ehdr *header, Elf64_Shdr *shdr, 
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

const Elf64_Sym *gnu_lookup(const char *strtab, const Elf64_Sym *symtab,
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

    /* If at least one bit is not set, a symbol is surely missing. */
    if ((word & mask) != mask) {
        return NULL;
    }

    uint32_t n = buckets[namehash % hashtab->nbuckets];
    if (n == 0)
        return NULL;

    const Elf64_Sym *sym = symtab + n;
    const uint32_t *hashval = &chain[n - hashtab->symoffset];

    /* Loop through the chain. */
    for (namehash &= ~1; 1; sym++) {
        const uint32_t hash = *hashval++;
        const char* symname = strtab + sym->st_name;

        if ((namehash == (hash & ~1)) && !strcmp(name, symname)) {
            printf("found\n");
            return sym;
        }

        if (hash & 1)
            break;
    }

    printf("Not found\n");
    return NULL;
}

const Elf64_Sym *find_symbol(Elf64_Ehdr *header)
{
    if (!header->e_shoff || !header->e_shentsize || 
        header->e_shstrndx == SHN_UNDEF)
        return NULL;

    //uint16_t shsize = header->e_shentsize;
    uint16_t shnum;
    uint16_t shstrndx;

    Elf64_Shdr *shdr = add_oft(header, header->e_shoff);
    get_sh_infos(header, shdr, &shnum, &shstrndx);

    Elf64_Sym *dynsymtab = NULL;
    char *shstrtab = add_oft(header, shdr[shstrndx].sh_offset); 
    char *dynstrtab;
    char *strtab;

    struct gnu_table *gnutable;
    uint64_t dynsize;
    Elf64_Dyn *dynamic;
    for(uint16_t i = header->e_shnum - 1; i > 0; --i){
        char* name = shstrtab + shdr[i].sh_name;
        printf("%s\n",name);
        if (!strcmp(name, ".strtab")) {
            printf("found strtab: %s\n", name);
            strtab = add_oft(header, shdr[i].sh_offset);
        } else if (!strcmp(name, ".dynamic")) {
            printf("found dynamic: %s\n", name);
            dynamic = add_oft(header, shdr[i].sh_offset);
            dynsize = shdr[i].sh_size / shdr[i].sh_entsize;
        }/* else if (!strcmp(name, ".gnu.hash")) {
            printf("found hash: %s\n", name);
            gnutable = add_oft(header, shdr[i].sh_offset);
        }*/

    }
#if 0
    Elf64_Phdr *phdr = add_oft(header, header->e_phoff);
    for (uint16_t i = 0; i < header->e_phnum; i++, phdr++)
        if (phdr->p_type == PT_DYNAMIC)
            break;

    Elf64_Dyn *dyn = add_oft(header, phdr->p_vaddr);
    const Elf64_Sym *ret = NULL;
    for (; dyn->d_tag; dyn++) {
        switch (dyn->d_tag) {
            case DT_STRTAB:
                dynstrtab = add_oft(header, dyn->d_un.d_ptr);
                break;
            case DT_SYMTAB:
                dynsymtab = add_oft(header, dyn->d_un.d_ptr);
                break;
            case DT_GNU_HASH:
            {
                gnutable = add_oft(header, dyn->d_un.d_ptr);
                ret = gnu_lookup(dynstrtab, dynsymtab, gnutable, "fflush");
            }
                break;
            default:
                break;
        }
     }

#endif
    for (uint64_t i = 0; i < dynsize; ++i)
    {
        if (dynamic[i].d_tag == DT_GNU_HASH) {
            printf("Found hash table at %zu\n", i);
            gnutable = add_oft(header, dynamic[i].d_un.d_ptr);
        } else if (dynamic[i].d_tag == DT_STRTAB) {
            printf("Found dynstrtab at %zu\n", i);
            dynstrtab = add_oft(header, dynamic[i].d_un.d_ptr);
        } else if (dynamic[i].d_tag == DT_SYMTAB) {
            printf("Found dynsymtab at %zu\n", i);
            dynsymtab = add_oft(header, dynamic[i].d_un.d_ptr);
        }

    }
    const Elf64_Sym *ret = gnu_lookup(dynstrtab, dynsymtab, gnutable, "instr_print");

    strtab = strtab; /* FIXME */
    for (uint64_t i = 0; dynstrtab[i + 1];)
    {
        i += printf("%s\n", dynstrtab + i);
    }
    if (ret)
        printf("FOUND\n");
    else
        printf("NOT FOUND\n");

    return ret;
}
