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
#include <util/hook/hook.hh>
#include <set>

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

        long long offset;            // offset
        std::string dev;        // device major:minor
        long long inode;              // inode of the file that backs the area
        std::string pathName;   //Path name to that executable

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

            ElfW(Addr) _addrStart = reinterpret_cast<ElfW(Addr)>(dt.addrStart);
            ElfW(Addr) _addrEnd = reinterpret_cast<ElfW(Addr)>(dt.addrEnd);

            os << std::noshowbase << std::hex << _addrStart << "-" << _addrEnd << " " << ss.str() << " ";
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
     * Group entry by its executable
     */
    class ExecEntryGroup {
        ssize_t pmEntryStartIndex;
        ssize_t pmEntryEndIndex;
    };

    /**
     * This class was a helper tool to parse /proc/self/maps
     * Current implementation uses STL API and may not the most efficient way. But it's fine for initialization and the code looks cleaner.
     */
    class PmParser_Linux : public Object {
    public:
        //This will be current executable name
        std::string curExecAbsolutePath;
        //Only path
        std::string curExecPath;
        //Only file name
        std::string curExecFileName;

        std::vector<PMEntry_Linux> pmEntryVector;

        PmParser_Linux(int procID = -1);

        /**
        * Determine whether current elf file use relative address or absolute address
        * @param curBaseAddr
        * @param curFileiD
        * @param targetAddr
        * @return
        */
        uint8_t *autoAddBaseAddr(uint8_t *curBaseAddr, FileID curFileiD, ElfW(Addr) targetAddr);


        void *readProcMem(void *startAddr, size_t bytes);

        /**
         * A convenient way to print /proc/{pid}/maps
         */
        void printPM();

        /**
         * Return addr is located in which file
         * @param fileId in fileIDMap
         */
        virtual ssize_t findExecNameByAddr(void *addr);

        ~PmParser_Linux() override;


        /**
         * Parse /proc/{pid}/maps into procMap
         */
        virtual bool parsePMMap();

    protected:
        //Process ID
        int procID;

        //The filestream for process file

        /**
         * Open /proc/{pid}/maps
         */
        virtual bool openPMMap(std::ifstream &file) const;

        virtual bool parseAddrStr(PMEntry_Linux &curEntry, const std::string &addrStr);

        virtual bool parseOffsetStr(PMEntry_Linux &curEntry, const std::string &offsetStr);

        virtual bool parsePermStr(PMEntry_Linux &curEntry, ssize_t curEntryIndex, const std::string &permStr);


    };

};


#endif
#endif
