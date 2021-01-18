
#ifndef SCALER_ELFPARSER_H
#define SCALER_ELFPARSER_H

#include <string>
#include <type/Object.hpp>
#include <link.h>
#include <vector>
#include <elf.h>
#include <map>

namespace scaler {
    //todo: Making ELF Parser a more complete class. (Make it possible to parse everything. ELF parser culd be a standalone module)
    class ELFParser : Object {
    public:
        std::string elfPath;
        std::map<std::string, size_t> relaFuncName;

        explicit ELFParser(std::string elfPath);

        ELFParser(ELFParser &) = delete;

        /**
         * @return A pointer to the start of ELF section specified by targetSecName. Nullptr if not found.
         * The memory is freed by ELFParser.
         */
        void *getSecPtr(std::string targetSecName);

        void *getSegPtr(ElfW(Word) segType);

        ElfW(Phdr) *getProgHdrByType(ElfW(Word) type);

        ElfW(Shdr) *getSecHdrByName(std::string targetSecName);

        long long int getSecLength(std::string targetSecName);


        std::vector<std::string> getSecNames();


        ~ELFParser() override;

        void parse();

    private:
        uint8_t *elfFile = nullptr;
        ElfW(Ehdr) *elfHdr = nullptr; //ELF Header
        ElfW(Shdr) *secHdr = nullptr; //Secion Header
        ElfW(Phdr) *progHdr;//Program Header
        const char *secStrtbl = nullptr;//Secion Name String Table
        ElfW(Half) shnum = 0;//The number of sections in section header
        ElfW(Half) phnum = 0;//The number of segments in program header
        ElfW(Rela) *relaPlt = nullptr;
        ElfW(Rela) *relaDyn = nullptr;
        std::map<std::string, size_t> secNameIndexMap;
        //todo: Use vector to represent sections as well. Add index array to each of them to make code cleaner.
        std::vector<ElfW(Phdr)> progHdrs;


        /**
        * Parse seciton header and program header for ELF
        */
        void parseELFHeaders();

        void closeELFFile();

        void openELFFile();


        void parseRelocationTable();


    };
}

#endif
