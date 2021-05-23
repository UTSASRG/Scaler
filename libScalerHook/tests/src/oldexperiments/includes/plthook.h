#ifndef PLTHOOK_H
#define PLTHOOK_H 1

#include <elf.h>
#include <stdio.h>

#define PLTHOOK_SUCCESS              0
#define PLTHOOK_FILE_NOT_FOUND       1
#define PLTHOOK_INVALID_FILE_FORMAT  2
#define PLTHOOK_FUNCTION_NOT_FOUND   3
#define PLTHOOK_INVALID_ARGUMENT     4
#define PLTHOOK_OUT_OF_MEMORY        5
#define PLTHOOK_INTERNAL_ERROR       6
#define PLTHOOK_NOT_IMPLEMENTED      7
#define R_JUMP_SLOT   R_X86_64_JUMP_SLOT
#define R_GLOBAL_DATA R_X86_64_GLOB_DAT

#define Elf_Plt_Rel   Elf_Rela
#define PLT_DT_REL    DT_RELA
#define PLT_DT_RELSZ  DT_RELASZ
#define PLT_DT_RELENT DT_RELAENT

#define SIZE_T_FMT "lu"
#define ELF_WORD_FMT "u"

#define ELF_XWORD_FMT "lu"
#define ELF_SXWORD_FMT "ld"
#define Elf_Half Elf64_Half
#define Elf_Xword Elf64_Xword
#define Elf_Sxword Elf64_Sxword
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Phdr Elf64_Phdr
#define Elf_Sym  Elf64_Sym
#define Elf_Dyn  Elf64_Dyn
#define Elf_Rel  Elf64_Rel
#define Elf_Rela Elf64_Rela
#define ELF_R_SYM ELF64_R_SYM
#define ELF_R_TYPE ELF64_R_TYPE

struct plthook {
    const Elf_Sym *dynsym;
    const char *dynstr;
    size_t dynstr_size;
    const char *plt_addr_base; //Difference between the address in the ELF file and the addresses in memory.
    const Elf_Plt_Rel *rela_plt;
    size_t rela_plt_cnt; //The count of PLT table
    const Elf_Plt_Rel *rela_dyn;
    size_t rela_dyn_cnt;
};
typedef struct plthook plthook_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Register plthook
 * If filename == null, open executable, if filename != null, open library.
 *
 * @param plthook_out Return created hook
 * @param filename Filename to a .so file
 * @return succeed or not
 */
int plthook_open(plthook_t **plthook_out, const char *filename);

/**
 * Register plthook by handle
 *
 * A handle, hanp, uniquely identifies a filesystem object or an entire
 * filesystem.  There is one and only one handle per filesystem or
 * filesystem object.
 *
 * @param plthook_out  Return created hook
 * @param handle A filename for .so
 * @return succeed or not
 */
int plthook_open_by_handle(plthook_t **plthook_out, void *handle);

/**
 * Register plthook by address
 * @param plthook_out  Return created hook
 * @param address Memory address to plthook
 * @return succeed or not
 */
int plthook_open_by_address(plthook_t **plthook_out, void *address);

/**
 * Enumerate plt hook
 * @param plthook Return created hook
 * @param pos  The index of plthook
 * @param name_out Output name
 * @param addr_out Output address
 * @return
 */
int plthook_enum(plthook_t *plthook, unsigned int *pos, const char **name_out, void ***addr_out);

/**
 * Replace plthook
 * @return
 */
int plthook_replace(plthook_t *plthook, const char *funcname, void *funcaddr, void **oldfunc);

/**
 * Unregister PLT Hook
 * @param plthook
 */
void plthook_close(plthook_t *plthook);

/**
 * Get the last error message
 * @return Error Message
 */
const char *plthook_error(void);

int plthook_open_real(plthook_t **plthook_out, struct link_map *lmap);

void *plthook_GetPltAddress(plthook_t **plthook_out, unsigned int funcId);


#ifdef __cplusplus
}; /* extern "C" */
#endif

#endif
