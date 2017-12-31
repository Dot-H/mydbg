#include <err.h>
#include <dwarf.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mapping.h"

static size_t dw_hash (void *namearg)
{
  const unsigned char *name = namearg;
  unsigned long h = 5381;
  unsigned char ch;

  while ((ch = *name++) != '\0')
    h = (h << 5) + h + ch;
  return h & 0xffffffff;
}

static int dw_cmp(void *keya, void *keyb)
{
    return !strcmp(keya, keyb);
}
static inline void *add_oft(const void *addr, const uint64_t oft)
{
    return (char *)addr + oft;
}

/**
** \param hdr Pointer to the head of a file's debug line
**
** \return Return a pointer to the file name table corresponding to
** \p hdr
*/
static char *get_file_name_table(const struct dw_hdrline *hdr)
{
    char *file = (char *)hdr;

    /* Directory table oft */
    file += sizeof(struct dw_hdrline);
    while (file[0] || file[1]) /* Search for double null byte */
        ++file;

    file += 2; /* Jump double null byte */
    return file;
}

/**
** \param nametab Pointer to a file name table.
**
** \return Return a pointer to the line number statements following
** \p nametab.
*/
static const uint8_t *get_line_statements(const char *nametab)
{
    const uint8_t *line_stmts = (uint8_t *)nametab;
    while (line_stmts[0] || line_stmts[4]) {
       if (!*line_stmts)
           line_stmts += 4; // Jump dir / time / size
       else
           ++line_stmts;
    }

    return line_stmts + 5; // Jump dir / time / size / nullbyte
}

static inline int8_t sleb128(int8_t val)
{
    val &= 0x7f;
    if (val > 0x40)
        return -(0x80 - val);

    return val;
}

static inline uint8_t leb128(uint8_t val) {
    return val & 0x7f;
}

/**
** \return return a value > 0 if \p opcodes corresponds to an end
** sequence and 0 otherwise.
*/
static inline int is_end_statement(const uint8_t *opcodes)
{
    return !opcodes[0] && opcodes[2] == DW_LNE_end_sequence;
}

static inline uint8_t add_addr(const struct dw_hdrline *hdr, uint8_t opcode) {
    return (opcode - hdr->opcode_base) / hdr->line_range;
}

static inline int8_t add_line(const struct dw_hdrline *hdr, uint8_t opcode) {
    return hdr->line_base + (opcode - hdr->opcode_base) % hdr->line_range;
}

static uint8_t hit_extd(struct dw_file *dw_file, uintptr_t *addr, 
                        const uint8_t *opcodes)
{
    uint8_t len = opcodes[0];
    if (opcodes[1] == DW_LNE_set_address) {
        memcpy(&dw_file->start, opcodes + 2, len - 1);
        *addr = dw_file->start;
        printf("addr: %lu\n", *addr);
    }

    return len + 1;
}

static uint8_t hit_std(const struct dw_hdrline *hdr, uintptr_t *addr,
                       size_t *line, const uint8_t *opcodes, uint8_t opcode)
{
    if (opcode == DW_LNS_advance_pc)
        *addr += leb128(opcodes[0]);
    else if (opcode == DW_LNS_advance_line)
        *line += sleb128(opcodes[0]);
    else if (opcode == DW_LNS_const_add_pc)
        *addr += add_addr(hdr, 255);
    else if (opcode == DW_LNS_fixed_advance_pc)
        *addr += leb128(opcodes[0]); // Should set op_index to 0 but we don't have any

    return hdr->std_opcodes[opcode - 1];
}

static void hit_spec(const struct dw_hdrline *hdr, uintptr_t *addr,
                     size_t *line, uint8_t opcode)
{
    *addr += add_addr(hdr, opcode);
    *line += add_line(hdr, opcode);
}

/**
** \return Return a filled struct dw_file corresponding to the
** informations inside \p line_stmts.
*/
static struct dw_file *parse_line_stmts(const struct dw_hdrline *hdr,
                                        const uint8_t *line_stmts)
{
    struct dw_file *dw_file = malloc(sizeof(struct dw_file));
    if (!dw_file)
        err(1, "Could not allocate a dw_file\n");

    const uint8_t *opcodes = line_stmts;
    uintptr_t addr = 0;
    size_t line    = 1;
    while (!is_end_statement(opcodes)) {
        uint8_t opcode = *opcodes++;
        if (!opcode)
            opcodes += hit_extd(dw_file, &addr, opcodes);
        else if (opcode < 13)
            opcodes += hit_std(hdr, &addr, &line, opcodes, opcode);
        else
            hit_spec(hdr, &addr, &line, opcode);
    }

    dw_file->end = addr;
    return dw_file;
}

struct htable *parse_debug_info(const void *elf, const struct dwarf *dwarf)
{
    if (!dwarf->debug_line)
        return NULL;

    struct htable *dw_table = dw_htable_creat();
    struct dw_hdrline *hdr;
    size_t len = 0;

    do {
        hdr = add_oft(elf, dwarf->debug_line->sh_offset + len);

        char *nametab = get_file_name_table(hdr);
        const uint8_t *line_stmts = get_line_statements(nametab);
        struct dw_file *dw_file = parse_line_stmts(hdr, line_stmts);

        dw_file->filename = nametab;
        dw_file->hdr      = hdr;
        dw_htable_insert(dw_file, dw_table);

        len += hdr->length + 4;
    } while (len < dwarf->debug_line->sh_size);

    return dw_table;
}

ssize_t get_line_from_addr(struct htable *dw_table, uintptr_t addr)
{
    struct dw_file *dw = dw_htable_search_by_addr(addr, dw_table);
    if (!dw)
        return -1;

    printf("addr: %zu\n", addr);
    printf("start: %zu\n", dw->start);
    printf("end: %zu\n", dw->end);

    char *nametab = get_file_name_table(dw->hdr);
    const uint8_t *line_stmts = get_line_statements(nametab);
    const uint8_t *opcodes = line_stmts;

    uintptr_t cur_addr = 0;
    size_t prv_line    = 0;
    size_t line        = 0;
    while (cur_addr < addr  || line == 0) {
        prv_line = line;
        uint8_t opcode = *opcodes++;
        if (!opcode)
            opcodes += hit_extd(dw, &cur_addr, opcodes);
        else if (opcode < 13)
            opcodes += hit_std(dw->hdr, &cur_addr, &line, opcodes, opcode);
        else
            hit_spec(dw->hdr, &cur_addr, &line, opcode);
    }

    return cur_addr == addr ? line : prv_line;
}

/****************************************/
/*      Wrappers to struct htable       */
/****************************************/

struct htable *dw_htable_creat(void)
{
   return htable_creat(dw_hash, DW_HTABLE_SIZE, dw_cmp);
}

void dw_htable_destroy(struct htable *htable)
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
            free(tmp->value);
            free(tmp);
            ++j;
        }
    }

    free(htable->array);
    free(htable);
}

struct dw_file *dw_htable_get(char *name, struct htable *htable)
{
    struct data *dw = htable_get(htable, name);
    if (!dw)
        return NULL;

    return dw->value;
}

struct dw_file *dw_htable_search_by_addr(uintptr_t addr,
                                         struct htable *htable) {
    for (size_t i = 0, j = 0; i < htable->size && j < htable->nmemb; ++i)
    {
        struct wl_list *head = &htable->array[i].link;
        struct data *tmp;
        wl_list_for_each(tmp, head, link) {
            struct dw_file *dw = tmp->value;
            if (dw->start <= addr && addr <= dw->end)
                return dw;

            ++j;
        }
    }   

    return NULL;
}

void dw_htable_insert(struct dw_file *dw, struct htable *htable)
{
    if (htable_insert(htable, dw, dw->filename) == -1)
        fprintf(stderr, "Could not insert %s in htable: already present\n",
                dw->filename);
}
