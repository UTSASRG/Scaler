#define _GNU_SOURCE

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/mman.h>
#include <errno.h>
#include <dlfcn.h>
#include <link.h>
#include <assert.h>
#include "plthook.h"


static char errmsg[512];
static size_t page_size;
#define ALIGN_ADDR(addr) ((void*)((size_t)(addr) & ~(page_size - 1)))

static int plthook_open_executable(plthook_t **plthook_out);

static int plthook_open_shared_library(plthook_t **plthook_out, const char *filename);

/**
 * Find Dynamic section of the shared object whose d_tag(Dynamic entry type) equals tag
 *
 * @param dyn Dynamic entry
 * @param tag d_tag(Dynamic entry type)
 * @return If found, return that dynamic entry. If not found, return NULL
 */
static const Elf_Dyn *find_dyn_by_tag(const Elf_Dyn *dyn, Elf_Sxword tag);

/**
 * https://www.cnblogs.com/pannengzhi/p/2018-04-09-about-got-plt.html
 * https://www.jianshu.com/p/5092d6d5caa3
 *
 * The real entry function to hook plt table.
 * plthook_open, plthook_open_by_handle, plthook_open_by_address, plthook_open_executable will all call this function
 * in the end
 *
 * It's purpose is to find corresponding sections and output them to
 *
 * @param plthook_out
 * @param lmap Linker variable that describes a loaded shared object. The `l_next' and `l_prev'
 * members form a chain of all the shared objects loaded at startup.
 * @return
 */
//static int plthook_open_real(plthook_t **plthook_out, struct link_map *lmap);

static void set_errmsg(const char *fmt, ...) __attribute__((__format__ (__printf__, 1, 2)));

int plthook_open(plthook_t **plthook_out, const char *filename) {
    *plthook_out = NULL;
    if (filename == NULL) {
        return plthook_open_executable(plthook_out);
    } else {
        return plthook_open_shared_library(plthook_out, filename);
    }
}

int plthook_open_by_handle(plthook_t **plthook_out, void *hndl) {
    struct link_map *lmap = NULL;

    if (hndl == NULL) {
        set_errmsg("NULL handle");
        return PLTHOOK_FILE_NOT_FOUND;
    }

    // Obtain information about a dynamically loaded object
    // hndl is the handle to that object
    // link_map is a chain of loaded objects
    // RTLD_DI_LINKMAP will return a link_map struct defined in <link.h>
    // https://code.woboq.org/userspace/glibc/include/link.h.html
    if (dlinfo(hndl, RTLD_DI_LINKMAP, &lmap) != 0) {
        set_errmsg("dlinfo error");
        return PLTHOOK_FILE_NOT_FOUND;
    }
    return plthook_open_real(plthook_out, lmap);
}

int plthook_open_by_address(plthook_t **plthook_out, void *address) {
    Dl_info info;
    struct link_map *lmap = NULL;

    *plthook_out = NULL;
    // dladdr, dladdr1 - translate address to symbolic information
    //
    if (dladdr1(address, &info, (void **) &lmap, RTLD_DL_LINKMAP) == 0) {
        set_errmsg("dladdr error");
        return PLTHOOK_FILE_NOT_FOUND;
    }
    return plthook_open_real(plthook_out, lmap);
}

static int plthook_open_executable(plthook_t **plthook_out) {
    /* Rendezvous structure used by the run-time dynamic linker to communicate
    details of shared object loading to the debugger.  If the executable's
    dynamic section has a DT_DEBUG element, the run-time linker sets that
    element's value to the address where this structure can be found.  */

    //https://github.com/Samsung/ADBI/blob/master/doc/DYNAMIC_LINKER
    return plthook_open_real(plthook_out, _r_debug.r_map);
}

static int plthook_open_shared_library(plthook_t **plthook_out, const char *filename) {
    //The function dlopen() loads the dynamic library file named by the null-terminated
    //string filename and returns an opaque "handle" for the dynamic library.

    //Perform lazy binding. Only resolve symbols as the code that references them is executed.
    //If the symbol is never referenced, then it is never resolved. (Lazy binding is only performed
    //for function references; references to variables are always immediately bound when the library is loaded.)
    void *hndl = dlopen(filename, RTLD_LAZY | RTLD_NOLOAD);
    struct link_map *lmap = NULL;

    if (hndl == NULL) {
        set_errmsg("dlopen error: %s", dlerror());
        return PLTHOOK_FILE_NOT_FOUND;
    }

    if (dlinfo(hndl, RTLD_DI_LINKMAP, &lmap) != 0) {
        set_errmsg("dlinfo error");
        dlclose(hndl);
        return PLTHOOK_FILE_NOT_FOUND;
    }
    dlclose(hndl);
    return plthook_open_real(plthook_out, lmap);
}

static const Elf_Dyn *find_dyn_by_tag(const Elf_Dyn *dyn, Elf_Sxword tag) {
    //In symbol table, the last entry is DT_NULL
    while (dyn->d_tag != DT_NULL) {
        if (dyn->d_tag == tag) {
            return dyn;
        }
        dyn++;
    }
    return NULL;
}

/**
 * Read /proc/self/maps to find memory permission
 * @param address memory address
 * @return
 */
static int get_memory_permission(void *address) {
    unsigned long addr = (unsigned long) address;
    FILE *fp;
    char buf[PATH_MAX];
    char perms[5];
    int bol = 1;

    fp = fopen("/proc/self/maps", "r");
    if (fp == NULL) {
        set_errmsg("failed to open /proc/self/maps");
        return 0;
    }
    while (fgets(buf, PATH_MAX, fp) != NULL) {
        unsigned long start, end;
        //strchr函数功能为在一个串中查找给定字符的第一个匹配之处
        int eol = (strchr(buf, '\n') != NULL);
        if (bol) {
            /* The fgets reads from the beginning of a line. */
            if (!eol) {
                /* The next fgets reads from the middle of the same line. */
                bol = 0;
            }
        } else {
            /* The fgets reads from the middle of a line. */
            if (eol) {
                /* The next fgets reads from the beginning of a line. */
                bol = 1;
            }
            continue;
        }

        if (sscanf(buf, "%lx-%lx %4s", &start, &end, perms) != 3) {
            continue;
        }
        if (start <= addr && addr < end) {
            int prot = 0;
            if (perms[0] == 'r') {
                prot |= PROT_READ;
            } else if (perms[0] != '-') {
                goto unknown_perms;
            }
            if (perms[1] == 'w') {
                prot |= PROT_WRITE;
            } else if (perms[1] != '-') {
                goto unknown_perms;
            }
            if (perms[2] == 'x') {
                prot |= PROT_EXEC;
            } else if (perms[2] != '-') {
                goto unknown_perms;
            }
            if (perms[3] != 'p') {
                goto unknown_perms;
            }
            if (perms[4] != '\0') {
                perms[4] = '\0';
                goto unknown_perms;
            }
            fclose(fp);
            return prot;
        }
    }
    fclose(fp);
    set_errmsg("Could not find memory region containing %p", (void *) addr);
    return 0;
    unknown_perms:
    fclose(fp);
    set_errmsg("Unexcepted memory permission %s at %p", perms, (void *) addr);
    return 0;
}


int plthook_open_real(plthook_t **plthook_out, struct link_map *lmap) {

    plthook_t plthook = {NULL,};
    const Elf_Dyn *dyn;
    const char *dyn_addr_base = NULL;

    if (page_size == 0) {
        page_size = sysconf(_SC_PAGESIZE); //Get the size of a page in bytes
    }

    plthook.plt_addr_base = (char *) lmap->l_addr; // l_addr: Shared library's load address

    //Section .dynamic: 保存了“动态链接器”所需要的信息，比如：
    //依赖哪些共享库
    //动态链接符号表位置
    //动态链接字符串表的位置
    //通过lmap->l_ld 可以获取到Dynamic section

    //Get Symbol table location (If an object file participates in dynamic linking,
    // its program header table will have an element of type PT_DYNAMIC.)
    dyn = find_dyn_by_tag(lmap->l_ld, DT_SYMTAB);
    if (dyn == NULL) {
        set_errmsg("failed to find DT_SYMTAB");
        return PLTHOOK_INTERNAL_ERROR;
    }
    plthook.dynsym = (const Elf_Sym *) (dyn_addr_base + dyn->d_un.d_ptr); //Symbol table location = base address + d_ptr

    dyn = find_dyn_by_tag(lmap->l_ld, DT_SYMENT); //The size, in bytes, of the DT_SYMTAB symbol entry.
    if (dyn == NULL) {
        set_errmsg("failed to find DT_SYMTAB");
        return PLTHOOK_INTERNAL_ERROR;
    }

    if (dyn->d_un.d_val != sizeof(Elf_Sym)) {
        set_errmsg("DT_SYMENT size %" ELF_XWORD_FMT " != %" SIZE_T_FMT, dyn->d_un.d_val, sizeof(Elf_Sym));
        return PLTHOOK_INTERNAL_ERROR;
    }

    // get .dynstr section
    dyn = find_dyn_by_tag(lmap->l_ld, DT_STRTAB);
    if (dyn == NULL) {
        set_errmsg("failed to find DT_STRTAB");
        return PLTHOOK_INTERNAL_ERROR;
    }
    plthook.dynstr = dyn_addr_base + dyn->d_un.d_ptr;

    // get .dynstr size
    dyn = find_dyn_by_tag(lmap->l_ld, DT_STRSZ);
    if (dyn == NULL) {
        set_errmsg("failed to find DT_STRSZ");
        return PLTHOOK_INTERNAL_ERROR;
    }
    plthook.dynstr_size = dyn->d_un.d_val;

    // https://www.zhihu.com/question/21249496
    // Get .rel.plt section
    dyn = find_dyn_by_tag(lmap->l_ld, DT_JMPREL);
    if (dyn != NULL) {
        //.rela.plt	 This section holds RELA type relocation information for the PLT section of a shared library or dynamically linked application.
        plthook.rela_plt = (const Elf_Plt_Rel *) (dyn_addr_base + dyn->d_un.d_ptr);
        dyn = find_dyn_by_tag(lmap->l_ld, DT_PLTRELSZ);
        if (dyn == NULL) {
            set_errmsg("failed to find DT_PLTRELSZ");
            return PLTHOOK_INTERNAL_ERROR;
        }
        plthook.rela_plt_cnt = dyn->d_un.d_val / sizeof(Elf_Plt_Rel);
    }
    // Get .rela.dyn
    dyn = find_dyn_by_tag(lmap->l_ld, PLT_DT_REL);
    if (dyn != NULL) {
        size_t total_size, elem_size;

        plthook.rela_dyn = (const Elf_Plt_Rel *) (dyn_addr_base + dyn->d_un.d_ptr);
        dyn = find_dyn_by_tag(lmap->l_ld, PLT_DT_RELSZ);

        if (dyn == NULL) {
            set_errmsg("failed to find PLT_DT_RELSZ");
            return PLTHOOK_INTERNAL_ERROR;
        }
        total_size = dyn->d_un.d_ptr;

        //Find the size of one Rela reloc
        dyn = find_dyn_by_tag(lmap->l_ld, PLT_DT_RELENT);
        if (dyn == NULL) {
            set_errmsg("failed to find PLT_DT_RELENT");
            return PLTHOOK_INTERNAL_ERROR;
        }
        elem_size = dyn->d_un.d_ptr;
        plthook.rela_dyn_cnt = total_size / elem_size; //Calculate # of entries in relocation table
    }

    if (plthook.rela_plt == NULL && plthook.rela_dyn == NULL) {
        set_errmsg("failed to find either of DT_JMPREL and DT_REL");
        return PLTHOOK_INTERNAL_ERROR;
    }

    *plthook_out = static_cast<plthook_t *>(malloc(sizeof(plthook_t)));
    if (*plthook_out == NULL) {
        set_errmsg("failed to allocate memory: %" SIZE_T_FMT " bytes", sizeof(plthook_t));
        return PLTHOOK_OUT_OF_MEMORY;
    }
    **plthook_out = plthook;
    return 0;
}

/**
 * Get name and absolute address of a relocation table entry
 */
static int check_rel(const plthook_t *plthook, const Elf_Plt_Rel *plt, Elf_Xword r_type,
                     const char **name_out, void ***addr_out) {
    if (ELF_R_TYPE(plt->r_info) == r_type) {
        // plt->r_info Relocation type and symbol index
        size_t idx = ELF_R_SYM(plt->r_info);
        idx = plthook->dynsym[idx].st_name;

        if (idx + 1 > plthook->dynstr_size) {
            set_errmsg("too big section header string table index: %" SIZE_T_FMT, idx);
            return PLTHOOK_INVALID_FILE_FORMAT;
        }
        *name_out = plthook->dynstr + idx;
        *addr_out = (void **) (plthook->plt_addr_base + plt->r_offset);
        return 0;
    }
    return -1;
}


int plthook_enum(plthook_t *plthook, unsigned int *pos, const char **name_out, void ***addr_out) {
    while (*pos < plthook->rela_plt_cnt) {
        const Elf_Plt_Rel *plt = plthook->rela_plt + *pos;
//        printf("Current Section ID: %d\n", ELF_R_TYPE(plt->r_info));
        int rv = check_rel(plthook, plt, R_JUMP_SLOT, name_out, addr_out);
        (*pos)++;
        if (rv >= 0) {
            return rv;
        }
    }
//    while (*pos < plthook->rela_plt_cnt + plthook->rela_dyn_cnt) {
//        const Elf_Plt_Rel *plt = plthook->rela_dyn + (*pos - plthook->rela_plt_cnt);
//        int rv = check_rel(plthook, plt, R_GLOBAL_DATA, name_out, addr_out);
//        (*pos)++;
//        if (rv >= 0) {
//            return rv;
//        }
//    }
    *name_out = NULL;
    *addr_out = NULL;
    return EOF;
}

int plthook_replace(plthook_t *plthook, const char *funcname, void *funcaddr, void **oldfunc) {
    size_t funcnamelen = strlen(funcname);
    unsigned int pos = 0;
    const char *name;
    void **addr;
    int rv;

    if (plthook == NULL) {
        set_errmsg("invalid argument: The first argument is null.");
        return PLTHOOK_INVALID_ARGUMENT;
    }
    while ((rv = plthook_enum(plthook, &pos, &name, &addr)) == 0) {
        if (strncmp(name, funcname, funcnamelen) == 0) {
            if (name[funcnamelen] == '\0' || name[funcnamelen] == '@') {
                int prot = get_memory_permission(addr);
                if (prot == 0) {
                    return PLTHOOK_INTERNAL_ERROR;
                }
                if (!(prot & PROT_WRITE)) {
                    //Change process memory using mprotect
                    if (mprotect(ALIGN_ADDR(addr), page_size, PROT_READ | PROT_WRITE) != 0) {
                        set_errmsg("Could not change the process memory permission at %p: %s",
                                   ALIGN_ADDR(addr), strerror(errno));
                        return PLTHOOK_INTERNAL_ERROR;
                    }
                }
                if (oldfunc) {
                    *oldfunc = *addr;
                }
                *addr = funcaddr;
                if (!(prot & PROT_WRITE)) {
                    mprotect(ALIGN_ADDR(addr), page_size, prot);
                }
                return 0;
            }
        }
    }
    if (rv == EOF) {
        set_errmsg("no such function: %s", funcname);
        rv = PLTHOOK_FUNCTION_NOT_FOUND;
    }
    return rv;
}

void plthook_close(plthook_t *plthook) {
    if (plthook != NULL) {
        free(plthook);
    }
}

const char *plthook_error(void) {
    return errmsg;
}

static void set_errmsg(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(errmsg, sizeof(errmsg) - 1, fmt, ap);
    va_end(ap);
}

void *plthook_GetPltAddress(plthook_t **plthook_out, unsigned int funcId) {
    const char *name;
    void **addr;
    unsigned int funcIdCpy=funcId;
    plthook_enum(*plthook_out, &funcIdCpy, &name, &addr);
    return *addr;
}