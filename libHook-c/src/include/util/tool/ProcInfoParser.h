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
#include <type/Object.hpp>
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

namespace scaler {

//The following class is declared and defined only under linux.

    /**
     * Represent a line in /prof/{pid}/map
     */
    struct PMEntry {
        enum PERM {
            EXEC = 0,
            WRITE = 1,
            READ = 2,
            PRIVATE = 3
        };

        uint8_t *addrStart;        // start address of the segment
        uint8_t *addrEnd;          // end address
        unsigned char permBits; // Is readable
        short fileId;

        inline bool isE() const {
            return permBits & (1 << PERM::EXEC);
        }

        inline bool isW() const {
            return permBits & (1 << PERM::WRITE);
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
    };

    /**
     * Group entry by its executable
     */
    class ExecEntryGroup {
        ssize_t pmEntryStartIndex;
        ssize_t pmEntryEndIndex;
    };

    typedef void (*FileNameCallBack)(const char *pathName, const ssize_t length, const ssize_t fileId);

    /**
     * This class was a helper tool to parse /proc/self/maps
     * Current implementation uses STL API and may not the most efficient way. But it's fine for initialization and the code looks cleaner.
     */
    class PmParser : public Object {
    public:
        PmParser();

        /**
        * Determine whether current elf file use relative address or absolute address
        * @param curBaseAddr
        * @param curFileiD
        * @param targetAddr
        * @return
        */
        uint8_t *autoAddBaseAddr(uint8_t *curBaseAddr, FileID curFileiD, ElfW(Addr) targetAddr);


        /**
         * A convenient way to print /proc/{pid}/maps
         */
        void printPM();

        /**
         * Return addr is located in which file
         * @param fileId in fileIDMap
         */
        virtual ssize_t findExecNameByAddr(void *addr);

        ~PmParser() override;


        /**
         * Parse /proc/{pid}/maps into procMap
         */
        virtual bool parsePMMap();

        Array<PMEntry> pmEntryArray;
        std::vector<std::string> fileNameArr;
    };

};


#endif
#endif
