
#ifndef SCALER_ELFPARSER_H
#define SCALER_ELFPARSER_H

#ifdef __linux

#include <string>
#include <type/Object.hpp>
#include <link.h>
#include <vector>
#include <elf.h>
#include <map>
#include <util/datastructure/Array.h>

#define ELFW(type)    _ElfW (ELF, __ELF_NATIVE_CLASS, type)
namespace scaler {
    //todo: Making ELF Parser a more complete class. (Make it possible to parse everything. ELF parser culd be a standalone module)
    class ELFParser : Object {
    public:

        //The path for current ELF file

        explicit ELFParser();

        bool parse(const char *elfPath);

        bool locateRequiredSecInMemory(const int secType, const std::string &secName, void *memStartAddr,
                                       ssize_t memCount, ssize_t keywordSize, void *&result);

        bool getSecHeader(const int secType, const std::string &secName, Elf64_Shdr &section);

        void getExtSymbolInfo(ssize_t &relaSymId, const char *&funcName, Elf64_Word &bind, Elf64_Word &type);

        ssize_t getExtSymbolStrOffset(ssize_t &relaSymId);


        Elf64_Addr getRelaOffset(const ssize_t &relaSymId) const;

        ELFParser(ELFParser &) = delete;


        ~ELFParser() override;


        Elf64_Shdr *secHdr = nullptr;
        ssize_t secHdrSize = 0;

        Elf64_Phdr *progHdr = nullptr;
        ssize_t progHdrSize = 0;

        const char *secHdrStrtbl = nullptr;//Secion Name String Table

        Elf64_Dyn *dynSection = nullptr;
        ssize_t dynSecSize = 0;

        Elf64_Rela *relaSection = nullptr;
        ssize_t relaEntrySize = 0;


        Elf64_Sym *dynSymTbl = nullptr;
        ssize_t dynSymTblSize = 0;

        const char *dynStrTbl = nullptr;
        ssize_t dynStrTblSize = 0;

        Elf64_Ehdr elfHdr;

        bool readSecContent(Elf64_Shdr &urSecHdr, void *&rltAddr, const ssize_t &oriSecSize);

        bool readSecContentWoMemReuse(Elf64_Shdr &curSecHdr, void *&rltAddr, const ssize_t &oriSecSize);

//        void *parseSecLoc(Elf64_Shdr &curHeader, uint8_t *baseStartAddr,uint8_t *possibleStartAddr,
//                          uint8_t *possibleEndAddr);


    protected:
        FILE *file = nullptr;

        bool verifyELFImg();

        bool readELFSectionHeader();

        bool readSecStrTable();

        bool readRelaEntries();

        inline bool readDynStrTable();

        inline bool readDynSymTable();


        bool verifyDynamicLibrary();
    };

}
#endif

#endif
