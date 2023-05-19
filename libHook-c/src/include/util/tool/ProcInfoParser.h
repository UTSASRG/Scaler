/*
 @Author	: ouadimjamal@gmail.com
 @date		: December 2015

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.  No representations are made about the suitability of this
software for any purpose.  It is provided "as is" without express or
implied warranty.

 */

#ifndef H_PMPARSER
#define H_PMPARSER


#ifdef __linux__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/limits.h>
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <link.h>
#include <elf.h>
#include <set>
#include <util/datastructure/Array.h>
#include <util/datastructure/Vector.h>
#include <type/ExtSymInfo.h>
#include <type/Object.hpp>

namespace scaler {

    //The following class is declared and defined only under linux.

    /**
     * Represent a line in /prof/{pid}/map
     */
    class PMEntry {
    public:
        enum PERM {
            READ = 3,
            WRITE = 2,
            EXEC = 1,
            PRIVATE = 0
        };
        // end address
        ssize_t fileId=-1;
        unsigned char permBits=0; // Is readable



        inline bool isR() const {
            return permBits & (1 << PERM::READ);
        }

        inline bool isW() const {
            return permBits & (1 << PERM::WRITE);
        }

        inline bool isE() const {
            return permBits & (1 << PERM::EXEC);
        }

        inline bool isP() const {
            return permBits & (1 << PERM::PRIVATE);
        }



        inline void setE() {
            permBits |= 1 << PERM::EXEC;
        }

        inline void setW() {
            permBits |= 1 << PERM::WRITE;
        }

        inline void setP() {
            permBits |= 1 << PERM::PRIVATE;
        }

        inline void setR() {
            permBits |= 1 << PERM::READ;
        }

        inline bool operator==(PMEntry& rho){
            return addrStart==rho.addrStart && addrEnd==rho.addrEnd && permBits==rho.permBits;
        }

        inline void setPermBits(char* permStr){
            //Parse permission
            permBits=0;
            if (permStr[0] == 'r') {
                setR();
            }
            if (permStr[1] == 'w') {
                setW();
            }
            if (permStr[2] == 'x') {
                setE();
            }
            if (permStr[3] == 'p') {
                setP();
            }
        }
        ssize_t loadingId=-1; //Marks the version of this entry, used to detect entry deletion
        uint8_t *addrStart=nullptr;
        // start address of the segment
        uint8_t *addrEnd=nullptr;
    };


    /**
     * Group entry by its executable
     */
    class FileEntry {
    public:
        ssize_t pathNameStartIndex=-1;
        ssize_t pathNameEndIndex=-1;
        ssize_t pmEntryNumbers;
        bool valid=false;
        ssize_t loadingId=-1; //Marks the version of this entry, used to detect entry deletion
        ssize_t getPathNameLength(){
            return pathNameEndIndex-pathNameStartIndex;
        }
    };

    typedef void (*FileNameCallBack)(const char *pathName, const ssize_t length, const ssize_t fileId);

    /**
     * This class was a helper tool to parse /proc/self/maps
     * Current implementation uses STL API and may not the most efficient way. But it's fine for initialization and the code looks cleaner.
     */
    class PmParser : public Object {
    public:
        explicit PmParser(std::string saveFolderName,std::string customProcFileName="");

        /**
        * Determine whether current elf file use relative address or absolute address
        * @param curBaseAddr
        * @param curFileiD
        * @param targetAddr
        * @return
        */
        virtual uint8_t *autoAddBaseAddr(uint8_t *curBaseAddr, FileID curFileiD, ElfW(Addr) targetAddr);


        /**
         * A convenient way to print /proc/{pid}/maps
         */
        virtual void printPM();

        /**
         * Return addr is located in which file
         * @param fileId in fileIDMap
         */
        virtual ssize_t findFileIdByAddr(void *addr);

        virtual void findPmEntryIdByAddr(void *addr,ssize_t& lo,bool& found);

        ~PmParser() override;


        /**
         * Parse /proc/{pid}/maps into procMap
         * Multiple invocation will keep the internal pmmap
         */
        virtual bool parsePMMap(ssize_t loadingId);

        Array<PMEntry> pmEntryArray;
        Array<FileEntry> fileEntryArray;

        std::vector<std::string> fileNameArr;
        std::string folderName;
    protected:
        pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
        std::string stringTable;
        std::string customProcFileName;
        ssize_t previousLoaidngId=-1;

    };


};


#endif
#endif
