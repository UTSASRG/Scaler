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

namespace scaler {

//The following class is declared and defined only under linux.

    /**
     * Represent a line in /prof/{pid}/map
     */
    class PMEntry_Linux : public Object {
    public:
        void *addrStart;        // start address of the segment
        void *addrEnd;          // end address
        unsigned long length;   // size of the segment
        bool isR;               // Is readable
        bool isW;               // Is writeable
        bool isE;               // Is executable
        bool isP;               // Is private

        long offset;            // offset
        std::string dev;        // device major:minor
        int inode;              // inode of the file that backs the area

        std::string pathName;   //Path name to that executable
        size_t fileId;          //Id of pathname in PmParser_Linux structure

        /**
         * Print PMEntry just like /proc/{pid}/map
         * You could:
         * PmEntry pmEntry;
         * cout<<pmEntry<<sendl;
         */
        friend std::ostream &operator<<(std::ostream &os, const PMEntry_Linux &dt) {

            std::ios_base::fmtflags oldState(os.flags());

            std::stringstream ss;
            ss << (dt.isR ? 'r' : '-');
            ss << (dt.isW ? 'w' : '-');
            ss << (dt.isE ? 'x' : '-');
            ss << (dt.isP ? 'p' : '-');

            ElfW(Addr) addrStart = reinterpret_cast<ElfW(Addr)>(dt.addrStart);
            ElfW(Addr) addrEnd = reinterpret_cast<ElfW(Addr)>(dt.addrEnd);

            os << std::noshowbase << std::hex << addrStart << "-" << addrEnd << " " << ss.str() << " ";
            os << std::setfill('0') << std::setw(8);
            os << dt.offset << " ";
            os << dt.dev << " ";
            os << std::dec << dt.inode << " ";
            os << dt.pathName;

            os.flags(oldState);
            return os;
        }

    };

    /**
     * This class was a helper tool to parse /proc/self/maps
     * Current implementation uses STL API and may not the most efficient way. But it's fine for initialization and the code looks cleaner.
     */
    class PmParser_Linux : public Object {
    public:
        //Map executable name with it's PMEntry
        std::map<std::string, std::vector<PMEntry_Linux>> procMap;
        // Used to find which fileID  floor(i/2) the corresponding fileID of pointer addrFileMap[i]
        // This array should be sorted by starting address for fast lookup
        std::vector<PMEntry_Linux> sortedSegments;

        //This will be current executable name
        std::string curExecFileName;

        // The id of a.so : fileIDMap[full path for a.so]
        std::map<std::string, size_t> fileIDMap;
        // Map id to file name
        std::vector<std::string> idFileMap;

        PmParser_Linux(int procID = -1);

        /**
         * A convenient way to print /proc/{pid}/maps
         */
        void printPM();

        /**
         * Return addr is located in which file
         * @param fileId in fileIDMap
         */
        size_t findExecNameByAddr(void *addr);


        ~PmParser_Linux() override;

    private:
        //Process ID
        int procID;

        //The filestream for process file
        std::ifstream file;

        /**
         * Open /proc/{pid}/maps
         */
        void openPMMap();

        /**
         * Parse /proc/{pid}/maps into procMap
         */
        void parsePMMap();
    };
}

#endif __linux__
#endif
