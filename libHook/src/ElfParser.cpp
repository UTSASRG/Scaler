#include <util/tool/ElfParser.h>

#include <util/tool/FileTool.h>
#include <elf.h>
#include <link.h>
#include <cstring>
#include <sstream>
#include <exceptions/ScalerException.h>
#include <exceptions/ErrCode.h>

namespace scaler {

    ELFParser_Linux::ELFParser_Linux(const std::string elfPath) {
        this->elfPath = elfPath;
    }

    bool ELFParser_Linux::parse() {
        if (!openELFFile())
            return false;

        //ELF header contains information the location of section table and program header table
        if (!readELFHeader()) {
            return false;
        }

        if (!readELFSecHeaders()) {
            return false;
        }

        if (!readELFProgHeaders()) {
            return false;
        }

        if (file != nullptr) {
            fclose(file);
            file = nullptr;
        }

        return true;
    }

    bool ELFParser_Linux::openELFFile() {
        file = fopen(elfPath.c_str(), "rb");
        if (file == NULL) {
            ERR_LOGS("Can't open ELF file \"%s\", reason:", strerror(errno));
            return false;
        }
        return true;
    }

    bool ELFParser_Linux::readELFHeader() {
        //Read ELF file to memory
        elfHdr = static_cast<ElfW(Ehdr) *>(malloc(sizeof(ElfW(Ehdr))));

        if (!elfHdr) {
            fatalError("Failed to allocate memory for elfHdr")
            return false;
        }

        if (!fread(elfHdr, 1, sizeof(ElfW(Ehdr)), file)) {
            ERR_LOGS("Failed to read elfHdr because: %s", strerror(errno));
            return false;
        }

        //Check whether ELF file is valid through magic number
        //This ELF Parser is only used for X86_64
        if (strncmp((const char *) elfHdr->e_ident, ELFMAG, SELFMAG) != 0 ||
            (elfHdr->e_ident[EI_CLASS] != ELFCLASS64) ||
            (elfHdr->e_ident[EI_DATA] != ELFDATA2LSB) ||
            (elfHdr->e_machine != EM_X86_64) ||
            (elfHdr->e_version != 1)) {
            ERR_LOGS("ELF magic number check failed. Is this a ELF file? Path:%s", elfPath.c_str());
            return false;
        }

        return true;
    }

    bool ELFParser_Linux::readELFSecHeaders() {

        //Read all section headers
        secHdr = static_cast<ElfW(Shdr) *>(calloc(elfHdr->e_shnum, sizeof(ElfW(Shdr))));
        if (!secHdr) {
            fatalError("Failed to allocate memory for secHdr");
            return false;
        }

        if (fseek(file, elfHdr->e_shoff, SEEK_SET)!=0) {
            ERR_LOGS("Failed to fseek because: %s", strerror(errno));
            return false;
        }

        if (!fread(secHdr, sizeof(ElfW(Shdr)), elfHdr->e_shnum, file)) {
            ERR_LOGS("Failed to read elfHdr because: %s", strerror(errno));
            return false;
        }

        //Read section name string table
        ElfW(Shdr) *strTblSecHdr = secHdr + elfHdr->e_shstrndx;
        secStrtbl = static_cast<const char *>(malloc(sizeof(char) * strTblSecHdr->sh_size));
        if (!secStrtbl) {
            fatalError("Failed to allocate memory for secStrtbl");
            return false;
        }

        if (fseek(file, strTblSecHdr->sh_offset, SEEK_SET)!=0) {
            ERR_LOGS("Failed to fseek because: %s", strerror(errno));
            return false;
        }

        if (!fread((void *) secStrtbl, 1, strTblSecHdr->sh_size, file)) {
            ERR_LOGS("Failed to read secStrtbl because: %s", strerror(errno));
            return false;
        }

        //Store section header into secNameIndexMap for faster lookup
        shnum = elfHdr->e_shnum;
        for (int i = 0; i < shnum; ++i) {
            ElfW(Shdr) &curShDr = secHdr[i];
            SecInfo curSecInfo;
            curSecInfo.secHdr = curShDr;
            curSecInfo.secId = i;
            secNameIndexMap[std::string(secStrtbl + curShDr.sh_name)] = curSecInfo;
        }

        return true;
    }

    bool ELFParser_Linux::readELFProgHeaders() {
        //Read all program headers
        progHdr = static_cast<ElfW(Phdr) *>(calloc(elfHdr->e_phnum, sizeof(ElfW(Phdr))));
        if (!progHdr) {
            ERR_LOG("Failed to allocate memory for progHdr");
            return false;
        }
        if (fseek(file, elfHdr->e_phoff, SEEK_SET)!=0) {
            ERR_LOGS("Failed to allocate memory for e_phnum, because: %s", strerror(errno));
            return false;
        }
        if (!fread(progHdr, sizeof(ElfW(Phdr)), elfHdr->e_phnum, file)) {
            ERR_LOGS("Failed to fseek because: %s", strerror(errno));
            return false;
        }

        //Store program header into segTypeIndexMap for faster lookup
        phnum = elfHdr->e_phnum;
        for (int i = 0; i < phnum; ++i) {
            auto &curProgHdr = progHdr[i];
            SegInfo curSegInfo;
            curSegInfo.progHdr = curProgHdr;
            curSegInfo.segId = i;
            segTypeIndexMap[curProgHdr.p_type].emplace_back(curSegInfo);
        }
        return true;
    }


    ELFParser_Linux::~ELFParser_Linux() {
        if (elfHdr){
            free(elfHdr);
            elfHdr=nullptr;
        }
        if (secHdr)
            free(secHdr);
        if (progHdr) {
            free(progHdr);
            progHdr= nullptr;
        }
        if (secStrtbl)
            free((void *) secStrtbl);
        for (auto &iter : secIdContentMap) {
            free(iter.second);
        }
        for (auto &iter : segIdContentMap) {
            free(iter.second);
        }
    }


    bool ELFParser_Linux::getSecNames(std::vector<std::string> &retSecName) {
        for (auto iter = secNameIndexMap.begin(); iter != secNameIndexMap.end(); ++iter) {
            retSecName.push_back(iter->first);
        }
        return true;
    }

    bool ELFParser_Linux::getSecHdrByName(std::string targetSecName, ELFParser_Linux::SecInfo &retSecInfo) {
        if (secNameIndexMap.count(targetSecName) == 0) {
            ERR_LOGS("Cannot find section %s in %s", targetSecName.c_str(), elfPath.c_str());
            return false;
        }
        retSecInfo = secNameIndexMap.at(targetSecName);
        return true;
    }


    bool ELFParser_Linux::getProgHdrByType(ElfW(Word) type, std::vector<ELFParser_Linux::SegInfo> &retSegInfoVec) {
        if (segTypeIndexMap.count(type) == 0) {
            ERR_LOGS("Cannot find segment of type %d", type);
            return false;
        }
        retSegInfoVec = segTypeIndexMap.at(type);
        return true;
    }


    void *ELFParser_Linux::getSecContent(const SecInfo &targetSecInfo) {
        //todo: memory leak
        if (!openELFFile()) {
            return nullptr;
        }

        void *targetSecHdrContent;
        if (secIdContentMap.count(targetSecInfo.secId) == 0) {
            //If targetSecInfo have not been loaded, read the elffile file and cache it

            targetSecHdrContent = malloc(targetSecInfo.secHdr.sh_size);
            if (!targetSecHdrContent) {
                fatalError("Failed to allocate memory for targetSecHdrContent");
                return nullptr;
            }

            if (fseek(file, targetSecInfo.secHdr.sh_offset, SEEK_SET)!=0) {
                ERR_LOGS("fseek failed because: %s", strerror(errno));
                return nullptr;
            }

            if (!fread(targetSecHdrContent, targetSecInfo.secHdr.sh_size, 1, file)) {
                ERR_LOGS("Failed to read section header because: %s", strerror(errno));
                return nullptr;
            }
            //Store address for faster lookup
            secIdContentMap[targetSecInfo.secId] = targetSecHdrContent;
        } else {
            //The content has been loaded before, return it directly
            targetSecHdrContent = secIdContentMap.at(targetSecInfo.secId);
        }
        if (file) {
            fclose(file);
        }
        return targetSecHdrContent;
    }

    void *ELFParser_Linux::getSegContent(const SegInfo &targetSegInfo) {
        if (!openELFFile()) {
            return nullptr;
        }

        void *targetSegHdrContent;
        if (secIdContentMap.count(targetSegInfo.segId) == 0) {
            //If targetSegInfo have not been loaded, read the elffile file and cache it

            targetSegHdrContent = malloc(targetSegInfo.progHdr.p_filesz);
            if (!targetSegHdrContent) {
                fatalError("Cannot allocate memory for targetSegHdrContent");
                return nullptr;
            }

            if (fseek(file, targetSegInfo.progHdr.p_offset, SEEK_SET)!=0) {
                ERR_LOGS("Failed to fseek because: %s", strerror(errno));
                return nullptr;
            }

            if (!fread(targetSegHdrContent, targetSegInfo.progHdr.p_filesz, 1, file)) {
                ERR_LOG("Faild to parse p_filesz");
                return nullptr;
            }
            //Store address for faster lookup
            segIdContentMap[targetSegInfo.segId] = targetSegHdrContent;
        } else {
            //The content has been loaded before, return it directly
            targetSegHdrContent = secIdContentMap.at(targetSegInfo.segId);
        }
        if (file) {
            fclose(file);
        }
        return targetSegHdrContent;
    }

    bool ELFParser_Linux::getSegContent(std::vector<SegInfo> &targetSegInfos, std::vector<void *> &retSegContent) {
        for (int i = 0; i < targetSegInfos.size(); ++i) {
            //Call getSegContent to load and cache individual segment
            retSegContent.emplace_back(getSegContent(targetSegInfos[i]));
        }
        return true;
    }

    ElfW(Dyn) *ELFParser_Linux::findDynEntryByTag(Elf64_Dyn *dyn, Elf64_Sxword tag) {
        //In symbol table, the last entry is DT_NULL
        while (dyn->d_tag != DT_NULL) {
            if (dyn->d_tag == tag) {
                return dyn;
            }
            dyn++;
        }
        return nullptr;
    }

}
