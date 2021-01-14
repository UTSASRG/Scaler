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

    }


    void *ELFParser::getSecPtr(std::string targetSecName) {
        //Iterate through sections
        for (int i = 0; i < shnum; ++i) {
            ElfW(Shdr) curShDr = secHdr[i];
            if (strncmp(targetSecName.c_str(), secStrtbl + curShDr.sh_name, targetSecName.size()) == 0) {
                return elfFile + curShDr.sh_offset;
            }
        }
        return nullptr;
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
        for (int i = 0; i < shnum; ++i) {
            ElfW(Shdr) curShDr = secHdr[i];
            secNameVec.push_back(std::string(secStrtbl + curShDr.sh_name));
        }
        return secNameVec;
    }

    void ELFParser::parse() {
        parseELFHeaders();
        parseRelocationTable();

    }

    void ELFParser::parseRelocationTable() {
        std::vector<std::string> secName = getSecNames();
        size_t relaPlt = getSecNameStrIdByName(".rela.plt");
        if (relaPlt == -1) {
            throwScalerException("Cannot find .rela.plt")
        }
        //Loop through relocation table that match this id.
        ElfW(Rela) *relaPltloc = getSecPtr(".rela.plt");

    }

    ElfW(Dyn) *ELFParser::findDynByTag(ElfW(Dyn) *dyn, ElfW(Sxword) tag) {
        while (dyn->d_tag != DT_NULL) {
            if (dyn->d_tag == tag) {
                return dyn;
            }
            dyn++;
        }
        return NULL;
    }

    size_t ELFParser::getSecNameStrIdByName(std::string name) {
        size_t posi = -1;
        for (int i = 0; i < shnum; ++i) {
            ElfW(Shdr) curShDr = secHdr[i];
            if (std::string(secStrtbl + curShDr.sh_name).compare(name) == 0) {
                posi = i;
                break;
            }
        }
        return posi;
    }

}