#include <err.h>
#include <fcntl.h>
#include <dwarf.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

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

struct dw_file *dw_file_creat(void)
{
    struct dw_file *new = malloc(sizeof(struct dw_file));
    if (!new)
        err(1, "Could not allocate a dw_file\n");

    new->filename = NULL;
    new->dir_idx  = 0;
    new->mfile    = NULL;
    new->msize    = 0;
    new->hdr      = NULL;
    new->start    = 1;
    new->end      = 0;

    return new;
}

void dw_file_destroy(struct dw_file *dw)
{
    if (dw->mfile && munmap(dw->mfile, dw->msize) == -1)
        warn("Could not unmap %s", dw->filename);

    free(dw);
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

static int get_dir_idx(const char *nametab)
{
    return nametab[strlen(nametab) + 1];
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

/**
** \brief convert \p val to a signed leb128 (7bits type).
*/
static inline int8_t sleb128(int8_t val)
{
    val &= 0x7f;
    if (val > 0x40)
        return -(0x80 - val);

    return val;
}

/**
** \brief convert \p val to a unsigned leb128 (7bits type).
*/
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

/**
** \brief Check if the opcode corresponds to DW_LNE_set_address and set the
** address if it is. Do nothing otherwise.
**
** \return Return the number of opcode to jump to get to the next opcode of
** interest.
*/
static uint8_t hit_extd(struct dw_file *dw_file, uintptr_t *addr,
                        const uint8_t *opcodes)
{
    uint8_t len = opcodes[0];
    if (opcodes[1] == DW_LNE_set_address) {
        memcpy(&dw_file->start, opcodes + 2, len - 1);
        *addr = dw_file->start;
    }

    return len + 1;
}

/**
** \brief actualize \p addr or \p line in function of the hit standard
** opcode as described in section 6.2.5.2 of http://dwarfstd.org/doc/DWARF4.pdf
**
** \return Return the number of argument of the standard opcode.
*/
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
        *addr += leb128(opcodes[0]); // Should set op_index to 0

    return hdr->std_opcodes[opcode - 1];
}

/**
** \brief actualize \p addr and \p line with the value \p opcode following
** the formula in section 6.2.5.1 of http://dwarfstd.org/doc/DWARF4.pdf
*/
static void hit_spec(const struct dw_hdrline *hdr, uintptr_t *addr,
                     size_t *line, uint8_t opcode)
{
    *addr += add_addr(hdr, opcode);
    *line += add_line(hdr, opcode);
}

/**
** \brief Interprete and consume the current opcode and its potential
** arguments. Actualize addr and line in function.
*/
static void actualize_opc(struct dw_file *dw_file, const uint8_t **opcodes,
                          size_t *addr, size_t *line)
{
    uint8_t opcode = **opcodes;
    *opcodes += 1;
    if (!opcode)
        *opcodes += hit_extd(dw_file, addr, *opcodes);
    else if (opcode < 13)
        *opcodes += hit_std(dw_file->hdr, addr, line, *opcodes, opcode);
    else
        hit_spec(dw_file->hdr, addr, line, opcode);
}

/**
** \return Return a filled struct dw_file corresponding to the
** informations inside \p line_stmts.
*/
static struct dw_file *parse_line_stmts(const struct dw_hdrline *hdr,
                                        const uint8_t *line_stmts)
{
    struct dw_file *dw_file = dw_file_creat();
    dw_file->hdr      = hdr;

    const uint8_t *opcodes = line_stmts;
    uintptr_t addr = 0;
    size_t line    = 1;
    while (!is_end_statement(opcodes))
        actualize_opc(dw_file, &opcodes, &addr, &line);

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
        const uint8_t *line_stmts = add_oft(hdr, hdr->prologue_len + PROL_COMP);
        struct dw_file *dw_file = parse_line_stmts(hdr, line_stmts);

        dw_file->filename = nametab;
        dw_file->dir_idx  = get_dir_idx(nametab);
        dw_htable_insert(dw_file, dw_table);

        len += hdr->length + 4; /* 4: Delimiter NULL bytes */
    } while (len < dwarf->debug_line->sh_size);

    return dw_table;
}

intptr_t get_line_addr(struct htable *dw_table, struct dw_file *dw,
                       char *filename, size_t lineno)
{
    if (!dw) {
        dw = dw_htable_get(filename, dw_table);
        if (!dw)
            return -1;
    }

    char *nametab = get_file_name_table(dw->hdr);
    const uint8_t *line_stmts = get_line_statements(nametab);
    const uint8_t *opcodes = line_stmts;

    uintptr_t addr = 0;
    size_t line    = 1;
    while (line < lineno && !is_end_statement(opcodes))
        actualize_opc(dw, &opcodes, &addr, &line);

    if (line != lineno)
        return -1;

    return addr;
}

ssize_t get_line_from_addr(struct htable *dw_table, uintptr_t addr,
                           struct dw_file **dw)
{
    *dw = dw_htable_search_by_addr(addr, dw_table);
    if (!*dw)
        return -1;

    char *nametab = get_file_name_table((*dw)->hdr);
    const uint8_t *line_stmts = get_line_statements(nametab);
    const uint8_t *opcodes = line_stmts;

    uintptr_t cur_addr = 0;
    size_t prv_line    = 1;
    size_t line        = 1;
    while ((cur_addr < addr || prv_line == line)
            && !is_end_statement(opcodes)) {
        prv_line = line;
        actualize_opc(*dw, &opcodes, &cur_addr, &line);
    }

    return (cur_addr == addr) ? line : prv_line;
}

intptr_t get_next_line_addr(struct htable *dw_table, uintptr_t addr,
                            struct dw_file **dw) {
    *dw = dw_htable_search_by_addr(addr, dw_table);
    if (!*dw)
        return -1;

    char *nametab = get_file_name_table((*dw)->hdr);
    const uint8_t *line_stmts = get_line_statements(nametab);
    const uint8_t *opcodes = line_stmts;

    uintptr_t cur_addr = 0;
    size_t prv_line    = 1;
    size_t line        = 1;
    while (cur_addr <= addr || prv_line == line) {
        prv_line = line;
        actualize_opc(*dw, &opcodes, &cur_addr, &line);
    }

    return cur_addr;
}

/**
** \brief open the \p dw's filename attribute, using the dir_idx attribute
** if different from 0.
**
** \return Return the opened file descriptor on success and -1 otherwise.
**
** \note In case of error, a message is print on stderr.
*/
static int dw_open(struct dw_file *dw)
{
    int fd = -1;
    if (dw->dir_idx == 0) {
        fd = open(dw->filename, O_RDONLY);
        if (fd == -1)
            warn("Could not open %s", dw->filename);
        return fd;
    }

    const char *dir = add_oft(dw->hdr, sizeof(struct dw_hdrline));
    size_t path_len = strlen(dir) + strlen(dw->filename) + 2; // '/' + '\0'
    char *path = malloc(path_len);
    if (!path) {
        close(fd);
        warn("Could not allocate path to open %s", dw->filename);
        return -1;
    }

    snprintf(path, path_len, "%s/%s", dir, dw->filename);
    printf("Open: %s\n", path);
    fd = open(path, O_RDONLY);
    if (fd == -1)
        warn("Could not open %s", path);

    free(path);
    return fd;
}

char *dw_map(struct dw_file *dw)
{
    int fd = dw_open(dw);
    if (fd == -1)
        return NULL;

    struct stat st;
    if (fstat(fd, &st) == -1) {
        warn("Could not stat %s\n", dw->filename);
        goto close_fd;
    }

    dw->msize = st.st_size;
    dw->mfile = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (dw->mfile == MAP_FAILED) {
        dw->mfile = NULL;
        dw->msize = 0;
        warn("Could not map %s\n", dw->filename);
    }

close_fd:
    if (close(fd) == -1)
        warn("Could not close %s", dw->filename);

    return dw->mfile;
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
            dw_file_destroy(tmp->value);
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
            if (dw->start <= addr && addr < dw->end)
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
