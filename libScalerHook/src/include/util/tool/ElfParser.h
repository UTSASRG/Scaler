
#ifndef SCALER_ELFPARSER_H
#define SCALER_ELFPARSER_H

#ifdef __linux

#include <string>
#include <type/Object.hpp>
#include <link.h>
#include <vector>
#include <elf.h>
#include <map>

#define ELFW(type)    _ElfW (ELF, __ELF_NATIVE_CLASS, type)
namespace scaler {
    //todo: Making ELF Parser a more complete class. (Make it possible to parse everything. ELF parser culd be a standalone module)
    class ELFParser_Linux : Object {

    public:


        class SecInfo {
        public:
            typedef ssize_t SecID;
            SecID secId;
            ElfW(Shdr) secHdr;
        };

        class SegInfo {
        public:
            typedef ssize_t SegID;
            SegID segId;
            ElfW(Phdr) progHdr;
        };

        //The path for current ELF file
        std::string elfPath;

        explicit ELFParser_Linux(std::string elfPath);

        bool parse();

        ELFParser_Linux(ELFParser_Linux &) = delete;

        /**
        * Find entry in Dyn, compare it's tag with parameter "tag"
        * @return Matched Dyn entry
        */
        ElfW(Dyn) *findDynEntryByTag(ElfW(Dyn) *dyn, ElfW(Sxword) tag);

        /**
         * @return A list of program header whose type is specified by param "type".
         */
        bool getProgHdrByType(ElfW(Word) type, std::vector<SegInfo> &retSegInfoVec);

        /**
         * @return A section header whose name is specified by param "targetSecName".
         */
        bool getSecHdrByName(std::string targetSecName, ELFParser_Linux::SecInfo &retSecInfo);

        /**
        * "Content" means the binary code for a specifc section
        * @return A pointer that points to the starting address of the section specified by targetSecName
        */
        void* getSecContent(const SecInfo &targetSecInfo);

        /**
        * "Content" means the binary code for a specifc segment
        * The returned memory will be freed with ELFParser
        */
        void* getSegContent(const SegInfo &targetSegInfo);

        /**
         * This is a convient method to get the address of a group of segments. (eg: segments with the same p_type)
         * The returned memory will be freed with ELFParser
        * @return A set of pointers that points to the starting address of the segment specified by a groups of segments
        */
        bool getSegContent(std::vector<SegInfo> &targetSegInfos, std::vector<void *> &retSegContent);

        /**
         * @return A list of section names
         */
        bool getSecNames(std::vector<std::string> &retSecName);

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
        std::map<SecInfo::SecID, void *> secIdContentMap; //Map section id to the pointer in ELF file

        //Map segment type a list of SegInfo with the same type
        std::map<ElfW(Word), std::vector<SegInfo>> segTypeIndexMap;
        std::map<SegInfo::SegID, void *> segIdContentMap; //Map segment id to the pointer in ELF file

        bool readELFHeader();

        bool readELFSecHeaders();

        bool readELFProgHeaders();

        /**
         * Open and read elf file to memory
         */
        bool openELFFile();


    };

}
#endif

#endif
