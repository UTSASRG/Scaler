#include <util/tool/ElfParser.h>
#include <util/tool/FileTool.h>
#include <elf.h>
#include <link.h>
#include <cstring>
#include <sstream>
#include <exceptions/ScalerException.h>

namespace scaler {

    ELFParser::ELFParser(const std::string elfPath) {
        this->elfPath = elfPath;
    }

    void ELFParser::openELFFile() {
        //Open ELF file
        FILE *file = fopen(elfPath.c_str(), "rb");
        if (file == NULL) {
            std::stringstream ss;
            ss << "Can't open ELF file \"" << elfPath.c_str() << "\"";
            throwScalerException(ss.str().c_str());
        }
        //Steak to the end of the file to determine the file size
        auto fileSize = getFileSize(file);

        //Read ELF file to memory
        elfFile = static_cast<uint8_t *>(malloc(fileSize));
        if (!elfFile) {
            throwScalerException("Memory allocation failed!");
        }

        //Read file to memory todo: Parsing without load whole ELF into memory
        fread(elfFile, 1, fileSize, file);
        fclose(file);
    }

    void ELFParser::closeELFFile() {
        if (elfFile) {
            free(elfFile);
            elfFile = nullptr;
        }
    }

    void ELFParser::parseELFHeaders() {
        openELFFile();

        // read the elfHdr
        elfHdr = reinterpret_cast<ElfW(Ehdr) *>(elfFile);

        //Check magic number
        if (memcmp(elfHdr->e_ident, ELFMAG, SELFMAG) != 0) {
            std::stringstream ss;
            ss << elfPath << " is not an ELF file";
            throwScalerException(ss.str().c_str());
        }



        //Read section header
        secHdr = reinterpret_cast<ElfW(Shdr) *>(elfFile + elfHdr->e_shoff);


        //Read section name string table
        ElfW(Shdr) *strTblSecHdr = secHdr + elfHdr->e_shstrndx;
        secStrtbl = reinterpret_cast<const char *>(elfFile + strTblSecHdr->sh_offset);



        //Read section header
        shnum = elfHdr->e_shnum;

        for (int i = 0; i < shnum; ++i) {
            ElfW(Shdr) *curShDr = &secHdr[i];
            secNameIndexMap[std::string(secStrtbl + curShDr->sh_name)] = i;
        }

    }


    void *ELFParser::getSecPtr(std::string targetSecName) {
        //Iterate through sections
        ElfW(Shdr) *curShDr = getSecHdrByName(targetSecName);
        return elfFile + curShDr->sh_offset;
    }

    ELFParser::~ELFParser() {
        closeELFFile();
    }

    long long int ELFParser::getSecLength(std::string targetSecName) {
        //Iterate through sections
        for (int i = 0; i < shnum; ++i) {
            ElfW(Shdr) curShDr = secHdr[i];
            if (targetSecName == secStrtbl + curShDr.sh_name) {
                return curShDr.sh_size;
            }
        }
        std::stringstream ss;
        ss << "Cannot get section \"" << targetSecName << "\"'s length from ELF file" << elfPath;
        throwScalerException(ss.str().c_str())
        return -1;
    }

    std::vector<std::string> ELFParser::getSecNames() {
        std::vector<std::string> secNameVec;
        for (auto iter = secNameIndexMap.begin(); iter != secNameIndexMap.end(); ++iter) {
            secNameVec.push_back(iter->first);
        }
        return secNameVec;
    }

    void ELFParser::parse() {
        parseELFHeaders();
        parseRelocationTable();

    }

    void ELFParser::parseRelocationTable() {
        //Find the starting point to .rela.plt.
        ElfW(Shdr) *relaPltSecHdr = getSecHdrByName(".rela.plt");
        ElfW(Rela) *relaPlt = reinterpret_cast<ElfW(Rela) *>(elfFile + relaPltSecHdr->sh_offset);
        if (!relaPlt) {
            throwScalerException("Cannot find .rela.plt")
        }

        //Find symbol table
        ElfW(Sym) *dynSymTbl = static_cast<ElfW(Sym) *>(getSecPtr(".dynsym"));

        const char *name;

        char *dynStrTbl = static_cast<char *>(getSecPtr(".dynstr"));

        size_t relaSecSize = relaPltSecHdr->sh_size / relaPltSecHdr->sh_entsize;
        for (int i = 0; i < relaSecSize; ++i) {
            //todo: ELF64_R_SYM is platform dependent
            name = dynStrTbl + (dynSymTbl + ELF64_R_SYM(relaPlt->r_info))->st_name;
            //The number of entries in a given table can be found by dividing the size of the table (given by sh_size
            //in the section header) by the size of each entry (given by sh_entsize).
            relaFuncName.emplace_back(name);

            relaPlt++;
        }


    }

    ElfW(Shdr) *ELFParser::getSecHdrByName(std::string targetSecName) {
        return &secHdr[secNameIndexMap[targetSecName]];
    }


}