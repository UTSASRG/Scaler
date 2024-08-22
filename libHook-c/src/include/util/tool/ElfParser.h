/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
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

        void getExtSymbolInfo(ssize_t &relaSymId, const char *&funcName, Elf64_Word &bind, Elf64_Word &type, Elf64_Rela *& relaSection);

        ssize_t getExtSymbolStrOffset(ssize_t &relaSymId, Elf64_Rela *& relaSection);


        Elf64_Addr getRelaOffset(const ssize_t &relaSymId, Elf64_Rela *& relaSection) const;

        ELFParser(ELFParser &) = delete;


        ~ELFParser() override;


        Elf64_Shdr *secHdr = nullptr;
        ssize_t secHdrSize = 0;

        Elf64_Phdr *progHdr = nullptr;
        ssize_t progHdrSize = 0;

        const char *secHdrStrtbl = nullptr;//Secion Name String Table

        Elf64_Dyn *dynSection = nullptr;
        ssize_t dynSecSize = 0;

        Elf64_Rela *relaPLTSection = nullptr;
        ssize_t relaPLTEntrySize = 0;
        ssize_t relaPLTSectionSize = 0;


        Elf64_Rela *relaDYNSection = nullptr;
        ssize_t relaDYNEntrySize = 0;
        ssize_t relaDYNSectionSize = 0;

        Elf64_Rela* pltSection = nullptr;
        ssize_t pltEntrySize = 0;
        ssize_t pltSectionSize = 0;

        Elf64_Rela* gotSection = nullptr;
        ssize_t gotEntrySize=0;
        ssize_t gotSectionSize = 0;


        Elf64_Sym *dynSymTbl = nullptr;
        ssize_t dynSymTblSize = 0;
        ssize_t dynSymSectionSize=0;

        const char *dynStrTbl = nullptr;
        ssize_t dynStrTblSize = 0;
        ssize_t dynStrSectionSize=0;

        Elf64_Ehdr elfHdr;

        bool readSecContent(Elf64_Shdr &urSecHdr, void *&rltAddr, ssize_t &oriSecSize);


//        void *parseSecLoc(Elf64_Shdr &curHeader, uint8_t *baseStartAddr,uint8_t *possibleStartAddr,
//                          uint8_t *possibleEndAddr);


    protected:
        FILE *file = nullptr;

        bool verifyELFImg();

        bool readELFSectionHeader();

        bool readSecStrTable();

        inline bool readRelaPLTEntries();

        inline bool readRelaDYNEntries();

        inline bool readDynStrTable();

        inline bool readDynSymTable();


        bool verifyDynamicLibrary();
    };

}
#endif

#endif
