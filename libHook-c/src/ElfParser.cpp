#include <util/tool/ElfParser.h>

#include <util/tool/FileTool.h>
#include <elf.h>
#include <link.h>
#include <cstring>
#include <sstream>
#include <exceptions/ScalerException.h>
#include <util/tool/MemoryTool.h>
#include <util/tool/Math.h>

namespace scaler {

    bool ELFParser::parse(const char *elfPath) {
        /**
         * Release previous memory allocations
         */
        if (secHdr) {
            delete secHdr;
            secHdr = nullptr;
        }

        if (file != nullptr) {
            fclose(file);
            file = nullptr;
        }

        file = fopen(elfPath, "rb");
        if (file == NULL) {
            ERR_LOGS("Can't open ELF file \"%s\", reason:", strerror(errno));
            return false;
        }

        //Read ELF header
        if (!fread(&elfHdr, 1, sizeof(Elf64_Ehdr), file)) {
            ERR_LOGS("Failed to read elfHdr because: %s", strerror(errno));
            return false;
        }

        //ELF header contains information the location of section table and program header table
        if (!verifyELFImg()) {
            return false;
        }

        if (!readELFSectionHeader()) {
            return false;
        }

        if (!readSecStrTable()) {
            return false;
        }

        if (!readDynStrTable()) {
            return false;
        }

        if (!readDynSymTable()) {
            return false;
        }

        if (!readRelaEntries()) {
            return false;
        }

        return true;
    }

    inline bool ELFParser::verifyELFImg() {
        //Check whether ELF file is valid through magic number
        //This ELF Parser is only used for X86_64
        if (strncmp((const char *) elfHdr.e_ident, ELFMAG, SELFMAG) != 0 ||
            (elfHdr.e_ident[EI_CLASS] != ELFCLASS64) ||
            (elfHdr.e_ident[EI_DATA] != ELFDATA2LSB) ||
            (elfHdr.e_machine != EM_X86_64) ||
            (elfHdr.e_version != 1)) {
            ERR_LOG("ELF format is not supported, parsing failed");
            return false;
        }
        return true;
    }

    inline bool ELFParser::readELFSectionHeader() {
        //Read all section headers+
        if (fseek(file, elfHdr.e_shoff, SEEK_SET) != 0) {
            ERR_LOGS("Failed to fseek because: %s", strerror(errno));
            return false;
        }
        secHdrSize = elfHdr.e_shnum;

        //Read all section headers
        secHdr = static_cast<Elf64_Shdr *>(malloc(elfHdr.e_shnum * sizeof(Elf64_Shdr)));
        if (!fread(secHdr, sizeof(ElfW(Shdr)), elfHdr.e_shnum, file)) {
            ERR_LOGS("Failed to read elfHdr because: %s", strerror(errno));
            return false;
        }
        return true;
    }

    inline bool ELFParser::readSecStrTable() {
        //Read section name string table
        ElfW(Shdr) &strTblSecHdr = secHdr[elfHdr.e_shstrndx];

        //Read string table
        if (fseek(file, strTblSecHdr.sh_offset, SEEK_SET) != 0) {
            ERR_LOGS("Failed to fseek because: %s", strerror(errno));
            return false;
        }

        secHdrStrtbl = static_cast<const char *>(malloc(sizeof(char) * strTblSecHdr.sh_size));
        if (!secHdrStrtbl) {
            fatalError("Failed to allocate memory for secStrtbl");
            return false;
        }
        if (!fread((void *) secHdrStrtbl, 1, strTblSecHdr.sh_size, file)) {
            ERR_LOGS("Failed to read secStrtbl because: %s", strerror(errno));
            return false;
        }
        return true;
    }


    ELFParser::~ELFParser() {
        if (secHdr)
            free(secHdr);
        if (secHdrStrtbl)
            free((void *) secHdrStrtbl);
        if (dynSection)
            free(dynSection);
        if (relaSection)
            free(relaSection);
        if (dynSymTbl)
            free(dynSymTbl);
        if (dynStrTbl)
            free((void *) dynStrTbl);
    }

    bool ELFParser::readSecContent(const int secType, const std::string &secName, Elf64_Shdr &curSecHdr, void *&rltAddr,
                                   const ssize_t &oriSecSize) {
        if (!getSecHeader(secType, secName, curSecHdr)) {
            ERR_LOG("Cannot find .dynsym section header");
            return false;
        }

        if (fseek(file, curSecHdr.sh_offset, SEEK_SET) != 0) {
            ERR_LOGS("Failed to fseek because: %s", strerror(errno));
            return false;
        }

        //Do not re-allocate memory if the current memory region is larger

        if (curSecHdr.sh_size > oriSecSize) {
            free(rltAddr);
            rltAddr = malloc(curSecHdr.sh_size);
            if (!rltAddr) {
                fatalErrorS("Cannot allocate memory for %s", secName.c_str());
            }
        } else {
            //Use the old memory section
        }


        if (!fread(rltAddr, curSecHdr.sh_size, 1, file)) {
            ERR_LOGS("Failed to read section header because: %s", strerror(errno));
            return false;
        }

        return true;

    }


    bool ELFParser::locateRequiredSecInMemory(const int secType, const std::string &secName, void *memStartAddr,
                                              ssize_t memCount, ssize_t keywordSize, void *&result) {
        Elf64_Shdr curShDr;
        if (!getSecHeader(secType, secName, curShDr)) {
            return false;
        }

        //Get section content
        void *targetSecHdrContent = malloc(curShDr.sh_size);
        if (!fread(targetSecHdrContent, curShDr.sh_size, 1, file)) {
            ERR_LOGS("Failed to read section header because: %s", strerror(errno));
            return false;
        }
        //Search the memory with memtool
        result = memSearch(memStartAddr, memCount, targetSecHdrContent,
                           min<ssize_t>(keywordSize, curShDr.sh_size));
        free(targetSecHdrContent);
        return true;
    }

    ELFParser::ELFParser() {

    }

    const char *ELFParser::getExtSymbolName(ssize_t &relaSymId) {
        ssize_t relIdx = ELF64_R_SYM(relaSection[relaSymId].r_info);
        ssize_t strIdx = dynSymTbl[relIdx].st_name;
        return dynStrTbl + strIdx;
    }

    inline bool ELFParser::readDynStrTable() {
        Elf64_Shdr curHeader;
        if (!readSecContent(SHT_STRTAB, ".dynstr", curHeader, (void *&) dynStrTbl, dynStrTblSize)) {
            ERR_LOG("Cannot read .dynstr");
            return false;
        }
        dynStrTblSize = curHeader.sh_size;
        return true;
    }

    bool ELFParser::readDynSymTable() {
        Elf64_Shdr curHeader;
        if (!readSecContent(SHT_DYNSYM, ".dynsym", curHeader, (void *&) dynSymTbl,
                            dynSymTblSize * sizeof(Elf64_Sym))) {
            ERR_LOG("Cannot read .dynsym");
            return false;
        }
        //dynSymTbl = static_cast<Elf64_Sym *>
        assert(curHeader.sh_entsize == sizeof(Elf64_Sym));
        dynSymTblSize = curHeader.sh_size / curHeader.sh_entsize;
        return true;
    }

    inline bool ELFParser::readRelaEntries() {
        Elf64_Shdr curHeader;
        if (!readSecContent(SHT_RELA, ".rela.plt", curHeader, (void *&) relaSection, relaEntrySize)) {
            ERR_LOG("Cannot read .rela.plt");
            return false;
        }
        assert(curHeader.sh_entsize == sizeof(Elf64_Rela));
        relaEntrySize = curHeader.sh_size / curHeader.sh_entsize;
        return true;
    }


    bool ELFParser::getSecHeader(const int secType, const std::string &secName, Elf64_Shdr &section) {
        for (int i = 0; i < secHdrSize; ++i) {
            if (secHdr[i].sh_type == secType &&
                strncmp(secHdrStrtbl + secHdr[i].sh_name, secName.c_str(), secName.size()) == 0) {
                section = secHdr[i];
                return true;
            }
        }
        return false;
    }


}
