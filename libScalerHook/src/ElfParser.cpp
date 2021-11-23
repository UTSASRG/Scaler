#include <util/tool/ElfParser.h>

#ifdef __linux

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
        openELFFile();
        //ELF header contains information the location of section table and program header table
        readELFHeader();
        readELFSecHeaders();
        readELFProgHeaders();
        fclose(file);
    }

    void ELFParser_Linux::openELFFile() {
        file = fopen(elfPath.c_str(), "rb");
        if (file == NULL) {
            throwScalerExceptionS(ErrCode::ELF_FILE_CANNOT_OPEN, "Can't open ELF file \"%s\"", elfPath.c_str());
        }
    }

    void ELFParser_Linux::readELFHeader() {
        //Read ELF file to memory
        elfHdr = static_cast<ElfW(Ehdr) *>(malloc(sizeof(ElfW(Ehdr))));
        if (!elfHdr) {
            throwScalerException(ErrCode::MEM_ALLOC_FAIL, "Faild to allocate memory for elfHdr");
        }
        if (!fread(elfHdr, 1, sizeof(ElfW(Ehdr)), file)) {
            throwScalerException(ErrCode::MEM_ALLOC_FAIL, "Faild to read elfHdr");
        }

        //Check whether ELF file is valid through magic number
        //This ELF Parser is only used for X86_64
        if (strncmp((const char *) elfHdr->e_ident, ELFMAG, SELFMAG) != 0 ||
            (elfHdr->e_ident[EI_CLASS] != ELFCLASS64) ||
            (elfHdr->e_ident[EI_DATA] != ELFDATA2LSB) ||
            (elfHdr->e_machine != EM_X86_64) ||
            (elfHdr->e_version != 1)) {
            throwScalerException(ErrCode::ELF_FILE_NOT_COMPATIBLE,
                                 "ELF type doesn't match. This ELF file maybe not compatible.");
        }
    }

    void ELFParser_Linux::readELFSecHeaders() {
        //Read all section headers
        secHdr = static_cast<ElfW(Shdr) *>(calloc(elfHdr->e_shnum, sizeof(ElfW(Shdr))));
        if (!secHdr) {
            throwScalerException(ErrCode::MEM_ALLOC_FAIL, "Faild to allocate memory for secHdr");
        }
        fseek(file, elfHdr->e_shoff, SEEK_SET);
        if (!fread(secHdr, sizeof(ElfW(Shdr)), elfHdr->e_shnum, file)) {
            throwScalerException(ErrCode::MEM_ALLOC_FAIL, "Faild to read secHdr");
        }

        //Read section name string table
        ElfW(Shdr) *strTblSecHdr = secHdr + elfHdr->e_shstrndx;
        secStrtbl = static_cast<const char *>(malloc(sizeof(char) * strTblSecHdr->sh_size));
        if (!secStrtbl) {
            throwScalerException(ErrCode::MEM_ALLOC_FAIL, "Faild to allocate memory for secStrtbl");
        }
        fseek(file, strTblSecHdr->sh_offset, SEEK_SET);
        if (!fread((void *) secStrtbl, 1, strTblSecHdr->sh_size, file)) {
            throwScalerException(ErrCode::MEM_ALLOC_FAIL, "Faild to read secStrtbl");
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

    }

    void ELFParser_Linux::readELFProgHeaders() {
        //Read all program headers
        progHdr = static_cast<ElfW(Phdr) *>(calloc(elfHdr->e_phnum, sizeof(ElfW(Phdr))));
        if (!progHdr) {
            throwScalerException(ErrCode::MEM_ALLOC_FAIL, "Faild to allocate memory for progHdr");
        }
        fseek(file, elfHdr->e_phoff, SEEK_SET);
        if (!fread(progHdr, sizeof(ElfW(Phdr)), elfHdr->e_phnum, file)) {
            throwScalerException(ErrCode::MEM_ALLOC_FAIL, "Faild to allocate memory for e_phnum");
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
    }


    ELFParser_Linux::~ELFParser_Linux() {
        if (elfHdr)
            free(elfHdr);
        if (secHdr)
            free(secHdr);
        if (progHdr)
            free(progHdr);
        if (secStrtbl)
            free((void *) secStrtbl);
        for (auto &iter : secIdContentMap) {
            free(iter.second);
        }
        for (auto &iter : segIdContentMap) {
            free(iter.second);
        }
    }


    std::vector<std::string> ELFParser_Linux::getSecNames() {
        std::vector<std::string> secNameVec;
        for (auto iter = secNameIndexMap.begin(); iter != secNameIndexMap.end(); ++iter) {
            secNameVec.push_back(iter->first);
        }
        return secNameVec;
    }

    ELFParser_Linux::SecInfo ELFParser_Linux::getSecHdrByName(std::string targetSecName) {
        if (secNameIndexMap.count(targetSecName) == 0) {
            throwScalerExceptionS(ErrCode::ELF_SECTION_NOT_FOUND, "Cannot find section %s in %s",
                                  targetSecName.c_str(), elfPath.c_str());
        }
        return secNameIndexMap.at(targetSecName);
    }


    std::vector<ELFParser_Linux::SegInfo> ELFParser_Linux::getProgHdrByType(ElfW(Word) type) {
        if (segTypeIndexMap.count(type) == 0) {
            throwScalerExceptionS(ErrCode::ELF_SEGMENT_NOT_FOUND, "Cannot find segment of type %d", type);
        }
        return segTypeIndexMap.at(type);
    }


    void *ELFParser_Linux::getSecContent(const SecInfo &targetSecInfo) {
        //todo: memory leak
        openELFFile();

        void *targetSecHdrContent;
        if (secIdContentMap.count(targetSecInfo.secId) == 0) {
            //If targetSecInfo have not been loaded, read the elffile file and cache it

            targetSecHdrContent = malloc(targetSecInfo.secHdr.sh_size);
            fseek(file, targetSecInfo.secHdr.sh_offset, SEEK_SET);
            if (!fread(targetSecHdrContent, targetSecInfo.secHdr.sh_size, 1, file)) {
                throwScalerException(ErrCode::ELF_SECTION_NOT_FOUND, "Faild to parse sh_size");
            }
            //Store address for faster lookup
            secIdContentMap[targetSecInfo.secId] = targetSecHdrContent;
        } else {
            //The content has been loaded before, return it directly

            targetSecHdrContent = secIdContentMap.at(targetSecInfo.secId);
        }
        fclose(file);
        return targetSecHdrContent;
    }

    void *ELFParser_Linux::getSegContent(const SegInfo &targetSegInfo) {
        openELFFile();

        void *targetSegHdrContent;
        if (secIdContentMap.count(targetSegInfo.segId) == 0) {
            //If targetSegInfo have not been loaded, read the elffile file and cache it

            targetSegHdrContent = malloc(targetSegInfo.progHdr.p_filesz);
            fseek(file, targetSegInfo.progHdr.p_offset, SEEK_SET);
            if(!fread(targetSegHdrContent, targetSegInfo.progHdr.p_filesz, 1, file)){
                throwScalerException(ErrCode::ELF_SECTION_NOT_FOUND, "Faild to parse p_filesz");
            }
            //Store address for faster lookup
            segIdContentMap[targetSegInfo.segId] = targetSegHdrContent;
        } else {
            //The content has been loaded before, return it directly

            targetSegHdrContent = secIdContentMap.at(targetSegInfo.segId);
        }
        fclose(file);
        return targetSegHdrContent;
    }

    std::vector<void *> ELFParser_Linux::getSegContent(std::vector<SegInfo> &targetSegInfos) {
        std::vector<void *> rltVec;
        for (int i = 0; i < targetSegInfos.size(); ++i) {
            //Call getSegContent to load and cache individual segment
            rltVec.emplace_back(getSegContent(targetSegInfos)[i]);
        }
        return rltVec;
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
#endif
