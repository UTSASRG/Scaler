
#ifndef SCALER_ELFPARSER_H
#define SCALER_ELFPARSER_H

#include <string>
#include <type/Object.hpp>
#include <link.h>
#include <vector>
#include <elf.h>

namespace scaler {
    class ELFParser : Object {
    public:
        std::string elfPath;

        ELFParser(std::string elfPath);

        ELFParser(ELFParser &) = delete;

        /**
         * @return A pointer to the start of ELF section specified by targetSecName. Nullptr if not found.
         * The memory is freed by ELFParser.
         */
        void *getSecPtr(std::string targetSecName);

        long long int getSecLength(std::string targetSecName);

        std::vector<std::string> getSecNames();

        size_t getSecNameStrIdByName(std::string);


        ~ELFParser() override;

        void parse();

    private:
        uint8_t *elfFile = nullptr;
        ElfW(Ehdr) *elfHdr = nullptr; //ELF Header
        ElfW(Shdr) *secHdr = nullptr; //Secion Header
        const char *secStrtbl = nullptr;//Secion Name String Table
        ElfW(Half) shnum = 0;//The number of sections in section header
        ElfW(Rela) *relaPlt = nullptr;
        ElfW(Rela) *relaDyn = nullptr;

        /**
        * Parse seciton header and program header for ELF
        */
        void parseELFHeaders();

        void closeELFFile();

        void openELFFile();


        void parseRelocationTable();

        ElfW(Dyn) *findDynByTag(ElfW(Dyn) *dyn, ElfW(Sxword) tag);
    };
}

#endif
