
#ifndef SCALER_ELFPARSER_H
#define SCALER_ELFPARSER_H

#ifdef __linux

#include <string>
#include <type/Object.hpp>
#include <link.h>
#include <vector>
#include <elf.h>
#include <map>

namespace scaler {
    //todo: Making ELF Parser a more complete class. (Make it possible to parse everything. ELF parser culd be a standalone module)
    class ELFParser_Linux : Object {

    public:
        enum ErrCode {
            SYMBOL_NOT_FOUND=-1
        };


        class SecInfo {
        public:
            size_t secId;
            ElfW(Shdr) secHdr;
        };

        class SegInfo {
        public:
            size_t segId;
            ElfW(Phdr) progHdr;
        };

        //The path for current ELF file
        std::string elfPath;

        explicit ELFParser_Linux(std::string elfPath);

        ELFParser_Linux(ELFParser_Linux &) = delete;

        /**
         * @return A list of program header whose type is specified by param "type".
         */
        std::vector<SegInfo> getProgHdrByType(ElfW(Word) type);

        /**
         * @return A section header whose name is specified by param "targetSecName".
         */
        SecInfo getSecHdrByName(std::string targetSecName);

        /**
        * "Content" means the binary code for a specifc section
        * @return A pointer that points to the starting address of the section specified by targetSecName
        */
        void *getSecContent(const SecInfo &targetSecInfo);

        /**
        * "Content" means the binary code for a specifc segment
        * @return A pointer that points to the starting address of the segment specified by targetSecName
        */
        void *getSegContent(const SegInfo &targetSegInfo);

        /**
         * This is a convient method to get the address of a group of segments. (eg: segments with the same p_type)
        * @return A set of pointers that points to the starting address of the segment specified by a groups of segments
        */
        std::vector<void *> getSegContent(std::vector<SegInfo> &targetSegInfos);

        /**
         * @return A list of section names
         */
        std::vector<std::string> getSecNames();

        ~ELFParser_Linux() override;


    protected:
        FILE *file;
        ElfW(Ehdr) *elfHdr = nullptr; //ELF Header
        ElfW(Shdr) *secHdr = nullptr; //Secion Header
        ElfW(Phdr) *progHdr;//Program Header
        const char *secStrtbl = nullptr;//Secion Name String Table
        ElfW(Half) shnum = 0;//The number of sections in section header
        ElfW(Half) phnum = 0;//The number of segments in program header
        std::map<std::string, SecInfo> secNameIndexMap; //Map section name to SecInfo for faster lookup
        std::map<size_t, void *> secIdContentMap; //Map section id to the pointer in ELF file

        //Map segment type a list of SegInfo with the same type
        std::map<ElfW(Word), std::vector<SegInfo>> segTypeIndexMap;
        std::map<size_t, void *> segIdContentMap; //Map segment id to the pointer in ELF file

        void readELFHeader();

        void readELFSecHeaders();

        void readELFProgHeaders();

        /**
         * Open and read elf file to memory
         */
        void openELFFile();


    };

}
#endif

#endif