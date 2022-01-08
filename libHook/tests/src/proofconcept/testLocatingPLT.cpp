#include <plthook.h>


#define _GNU_SOURCE

#include <elf.h>
#include <link.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <pmparser.h>
#include <string>


void readElfSections(const char *elfFile) {
    // Either Elf64_Ehdr or Elf32_Ehdr depending on architecture.

    FILE *file = fopen(elfFile, "rb");

    //Steak to the end of the file to determine the file size
    fseek(file, 0L, SEEK_END);
    long int fileSize = ftell(file);
    fseek(file, 0L, SEEK_SET);
    char *elfFileBin = (char *) malloc(fileSize);
    //Read the file
    fread(elfFileBin, 1, fileSize, file);

    if (file) {
        // read the elfHdr
        ElfW(Ehdr) *elfHdr = reinterpret_cast<Elf64_Ehdr *>(elfFileBin);
        assert (memcmp(elfHdr->e_ident, ELFMAG, SELFMAG) == 0);         // check so its really an elf file

        ElfW(Shdr) *shHdr = reinterpret_cast<Elf64_Shdr *>(elfFileBin + elfHdr->e_shoff);

        //Read section name string table
        ElfW(Shdr) *shStrTabHdr = shHdr + elfHdr->e_shstrndx;
        const char *const shStrtab = elfFileBin + shStrTabHdr->sh_offset;

        // read section header stirng table
        ElfW(Half) shHeaderEntrySize = elfHdr->e_shentsize;
        long int shnum = elfHdr->e_shnum;

        //Iterate through sections
        for (int i = 0; i < shnum; ++i) {
            ElfW(Shdr) curShDr = shHdr[i];
            printf("Name:%s \t Offset:%ld \t Size:%ld \n", shStrtab + curShDr.sh_name, curShDr.sh_offset,
                   curShDr.sh_size);
        }

        // read section header
        ElfW(Shdr) sectionHeader;
        fseek(file, elfHdr->e_shoff, SEEK_SET);
        fread(&shHdr, sizeof(shHdr), 1, file);

//        elfHdr.e_shentsize
//        elfHdr.e_shnum
//        elfHdr
//        elfHdr.e_shoff
//        elfHdr.e_shstrndx

        // finally close the file
        fclose(file);
    }

    free(elfFileBin);
}


void searchPLTContent(const char *elfFile) {
    // Either Elf64_Ehdr or Elf32_Ehdr depending on architecture.

    FILE *file = fopen(elfFile, "rb");

    //Steak to the end of the file to determine the file size
    fseek(file, 0L, SEEK_END);
    long int fileSize = ftell(file);
    fseek(file, 0L, SEEK_SET);
    char *elfFileBin = (char *) malloc(fileSize);
    //Read the file
    fread(elfFileBin, 1, fileSize, file);

    if (file) {
        // read the elfHdr
        ElfW(Ehdr) *elfHdr = reinterpret_cast<Elf64_Ehdr *>(elfFileBin);
        assert (memcmp(elfHdr->e_ident, ELFMAG, SELFMAG) == 0);         // check so its really an elf file

        ElfW(Shdr) *shHdr = reinterpret_cast<Elf64_Shdr *>(elfFileBin + elfHdr->e_shoff);

        //Read section name string table
        ElfW(Shdr) *shStrTabHdr = shHdr + elfHdr->e_shstrndx;
        const char *const shStrtab = elfFileBin + shStrTabHdr->sh_offset;

        // read section header stirng table
        ElfW(Half) shHeaderEntrySize = elfHdr->e_shentsize;
        long int shnum = elfHdr->e_shnum;

        char *firstBytes4PLT = nullptr;
        char *firstBytes4PLTSEC = nullptr;

        //Iterate through sections
        for (int i = 0; i < shnum; ++i) {
            ElfW(Shdr) curShDr = shHdr[i];
            const char *tableName = shStrtab + curShDr.sh_name;
            if (strncmp(tableName, ".plt", 4) == 0) {
                //PLT table!
                size_t bytesToSearch = std::min(curShDr.sh_size, (ElfW(Xword)) 12);
                if (bytesToSearch > 0) {

                }
            } else if (strncmp(tableName, ".plt.sec", 4) == 0) {

            }
//            printf("Name:%s \t Offset:%ld \t Size:%ld \n", , curShDr.sh_offset,
//                   curShDr.sh_size);
        }

        // read section header
        ElfW(Shdr) sectionHeader;
        fseek(file, elfHdr->e_shoff, SEEK_SET);
        fread(&shHdr, sizeof(shHdr), 1, file);

//        elfHdr.e_shentsize
//        elfHdr.e_shnum
//        elfHdr
//        elfHdr.e_shoff
//        elfHdr.e_shstrndx

        // finally close the file
        fclose(file);
    }

    free(elfFileBin);
}

uint8_t *readElfSection(uint8_t *elfFileBin, std::string segName) {
    // read the elfHdr
    ElfW(Ehdr) *elfHdr = reinterpret_cast<Elf64_Ehdr *>(elfFileBin);
    assert (memcmp(elfHdr->e_ident, ELFMAG, SELFMAG) == 0);         // check so its really an elf file

    ElfW(Shdr) *shHdr = reinterpret_cast<Elf64_Shdr *>(elfFileBin + elfHdr->e_shoff);

    //Read section name string table
    ElfW(Shdr) *shStrTabHdr = shHdr + elfHdr->e_shstrndx;
    uint8_t *shStrtab = elfFileBin + shStrTabHdr->sh_offset;

    // read section header stirng table
    ElfW(Half) shHeaderEntrySize = elfHdr->e_shentsize;
    long int shnum = elfHdr->e_shnum;

    //Iterate through sections
    for (int i = 0; i < shnum; ++i) {
        ElfW(Shdr) curShDr = shHdr[i];
        const char *sectionName = reinterpret_cast<const char *>(shStrtab + curShDr.sh_name);
        if (strncmp(sectionName, segName.c_str(), segName.length()) == 0) {
            return elfFileBin + curShDr.sh_offset;
        }
    }

}


uint8_t *searchSectionLocation(uint8_t *startArr, size_t range, uint8_t *targetData, size_t targetDataSize) {
    int i = 0, j = 0;
    uint8_t *beg = nullptr;
    while (i < range && j < targetDataSize) {
        if (startArr[i] == targetData[j]) {
            if (beg == nullptr) {
                beg = startArr + i;
            }
            ++j;
        } else {
            beg = nullptr;
            j = 0;
        }
        ++i;
    }
    return j == targetDataSize ? beg : nullptr;
}

void processSelfMap() {
    procmaps_iterator *maps = pmparser_parse(-1);
    //iterate over areas
    procmaps_struct *maps_tmp = NULL;

    const char *pathName = "";
    uint8_t *elfFileBin = nullptr;
    bool foundPLT = false;
    while ((maps_tmp = pmparser_next(maps)) != NULL) {
        if (foundPLT && strncmp(pathName, maps_tmp->pathname, strlen(maps_tmp->pathname)) == 0) {
            continue;
        }

        //Read file
        if (strncmp(pathName, maps_tmp->pathname, strlen(maps_tmp->pathname)) != 0) {
            foundPLT = false;
            if (elfFileBin != nullptr) {
                free(elfFileBin);
                elfFileBin = nullptr;
            }
            FILE *file = fopen(maps_tmp->pathname, "rb");
            if (file) {
                //Steak to the end of the file to determine the file size
                fseek(file, 0L, SEEK_END);
                long int fileSize = ftell(file);
                fseek(file, 0L, SEEK_SET);
                elfFileBin = (uint8_t *) malloc(fileSize);
                //Read the file
                fread(elfFileBin, 1, fileSize, file);
            }
        }
        if (elfFileBin == nullptr)
            continue;
        //Get section header
        uint8_t *pltSection = readElfSection(elfFileBin, ".plt");
        assert(pltSection != nullptr);
        uint8_t *pltSecSection = readElfSection(elfFileBin, ".plt.sec");
        assert(pltSecSection != nullptr);

        uint8_t *targetMem = static_cast<uint8_t *>(maps_tmp->addr_start);

        if (maps_tmp->is_x) {
            uint8_t *realPltAddr = searchSectionLocation(targetMem,
                                                         (uint8_t *) maps_tmp->addr_end -
                                                         (uint8_t *) maps_tmp->addr_start,
                                                         pltSection, 16);
            if (realPltAddr) {
                foundPLT = true;
                printf("Found .plt at addr %p inside %p-%p for %s\n",realPltAddr, maps_tmp->addr_start, maps_tmp->addr_end,
                       maps_tmp->pathname);
            }
        }

    }
    //mandatory: should free the list
    pmparser_free(maps);
}


//Reference
extern char __startplt, __startpltgot, __startpltsec, __rela_iplt_start, __rela_iplt_end;
extern char *ptrPlt = &__startplt, *ptrPltgot = &__startpltgot, *ptrPltsec = &__startpltsec, *ptrIplt = &__rela_iplt_start, *ptrIpltEnd = &__rela_iplt_end;

int main(int argc, char **argv) {
    printf("PltHook Pointer Addr: %p\n", ptrPlt);

    //https://stackoverflow.com/questions/55643058/finding-the-rendezvous-struct-r-debug-structure-of-a-process
//    dl_iterate_phdr(callback, NULL);
    r_debug *_myRdebug = nullptr;
    const ElfW(Dyn) *dyn = _DYNAMIC;
    for (dyn = _DYNAMIC; dyn->d_tag != DT_NULL; ++dyn)
        if (dyn->d_tag == DT_DEBUG)
            _myRdebug = (struct r_debug *) dyn->d_un.d_ptr;
    printf("_r1_debug.r_map %p\n", _myRdebug->r_map);


    printf("\nParsing ELF Sections..... \n");
//    readElfSections(argv[0]);

//    printf("\nParsing ELF Segments..... \n");
//    readElfSegments(argv[0]);

//    printFile("/proc/self/maps");
    processSelfMap();

    return 0;
}
