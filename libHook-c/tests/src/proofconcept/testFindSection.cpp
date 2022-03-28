#define _GNU_SOURCE
#include <link.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

const char *type_str(ElfW(Word) type)
{
    switch (type)
    {
        case PT_NULL:
            return "PT_NULL"; // should not be seen at runtime, only in the file!
        case PT_LOAD:
            return "PT_LOAD";
        case PT_DYNAMIC:
            return "PT_DYNAMIC";
        case PT_INTERP:
            return "PT_INTERP";
        case PT_NOTE:
            return "PT_NOTE";
        case PT_SHLIB:
            return "PT_SHLIB";
        case PT_PHDR:
            return "PT_PHDR";
        case PT_TLS:
            return "PT_TLS";
        case PT_GNU_EH_FRAME:
            return "PT_GNU_EH_FRAME";
        case PT_GNU_STACK:
            return "PT_GNU_STACK";
        case PT_GNU_RELRO:
            return "PT_GNU_RELRO";
        case PT_SUNWBSS:
            return "PT_SUNWBSS";
        case PT_SUNWSTACK:
            return "PT_SUNWSTACK";
        default:
            if (PT_LOOS <= type && type <= PT_HIOS)
            {
                return "Unknown OS-specific";
            }
            if (PT_LOPROC <= type && type <= PT_HIPROC)
            {
                return "Unknown processor-specific";
            }
            return "Unknown";
    }
}

const char *flags_str(ElfW(Word) flags)
{
    switch (flags & (PF_R | PF_W | PF_X))
    {
        case 0 | 0 | 0:
            return "none";
        case 0 | 0 | PF_X:
            return "x";
        case 0 | PF_W | 0:
            return "w";
        case 0 | PF_W | PF_X:
            return "wx";
        case PF_R | 0 | 0:
            return "r";
        case PF_R | 0 | PF_X:
            return "rx";
        case PF_R | PF_W | 0:
            return "rw";
        case PF_R | PF_W | PF_X:
            return "rwx";
    }
    __builtin_unreachable();
}

static int callback(struct dl_phdr_info *info, size_t size, void *data)
{
    int j;
    (void)data;

    printf("object \"%s\"\n", info->dlpi_name);
    printf("  base address: %p\n", (void *)info->dlpi_addr);
    if (size > offsetof(struct dl_phdr_info, dlpi_adds))
    {
        printf("  adds: %lld\n", info->dlpi_adds);
    }
    if (size > offsetof(struct dl_phdr_info, dlpi_subs))
    {
        printf("  subs: %lld\n", info->dlpi_subs);
    }
    if (size > offsetof(struct dl_phdr_info, dlpi_tls_modid))
    {
        printf("  tls modid: %zu\n", info->dlpi_tls_modid);
    }
    if (size > offsetof(struct dl_phdr_info, dlpi_tls_data))
    {
        printf("  tls data: %p\n", info->dlpi_tls_data);
    }
    printf("  segments: %d\n", info->dlpi_phnum);

    for (j = 0; j < info->dlpi_phnum; j++)
    {
        const ElfW(Phdr) *hdr = &info->dlpi_phdr[j];
        printf("    segment %2d\n", j);
        printf("      type: 0x%08X (%s)\n", hdr->p_type, type_str(hdr->p_type));
        printf("      file offset: 0x%08zX\n", hdr->p_offset);
        printf("      virtual addr: %p\n", (void *)hdr->p_vaddr);
        printf("      physical addr: %p\n", (void *)hdr->p_paddr);
        printf("      file size: 0x%08zX\n", hdr->p_filesz);
        printf("      memory size: 0x%08zX\n", hdr->p_memsz);
        printf("      flags: 0x%08X (%s)\n", hdr->p_flags, flags_str(hdr->p_flags));
        printf("      align: %zd\n", hdr->p_align);
        if (hdr->p_memsz)
        {
            printf("      derived address range: %p to %p\n",
                   (void *) (info->dlpi_addr + hdr->p_vaddr),
                   (void *) (info->dlpi_addr + hdr->p_vaddr + hdr->p_memsz));
        }
    }
    return 0;
}


int main(void)
{
    dl_iterate_phdr(callback, NULL);

    exit(EXIT_SUCCESS);
}