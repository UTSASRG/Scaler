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
        FileID fileId;          //Id of pathname in PmParser_Linux structure

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
        // This array should be sorted by starting address for fast lookup (binary search in findExecNameByAddr)
        std::vector<std::pair<FileID, PMEntry_Linux>> sortedSegments;


        //This will be current executable name
        std::string curExecAbsolutePath;
        //Only path
        std::string curExecPath;
        //Only file name
        std::string curExecFileName;


        // The id of a.so : fileIDMap[full path for a.so]
        std::map<std::string, FileID> fileIDMap;

        //The base address of an executable
        std::vector<std::pair<uint8_t *, uint8_t *>> fileBaseAddrMap;

        std::map<uint8_t *, FileID> startAddrFileMap;


        // Map id to file name
        std::vector<std::string> idFileMap;

        std::vector<FileID> linkedFileID;

        //For .plt, .plt.sec, we only need to search in segments with executable permission
        std::vector<PMEntry_Linux> executableSegments;
        //For _DYNAMIC, we only need to search in segments with readable but not executable permission
        std::vector<PMEntry_Linux> readableSegments;


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
        virtual int findExecNameByAddr(void *addr);

        ~PmParser_Linux() override;


        /**
         * Parse /proc/{pid}/maps into procMap
         */
        virtual bool parsePMMap();

        bool addrInApplication(void *addr);

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

        virtual bool parsePermStr(PMEntry_Linux &curEntry, const std::string &permStr);

        virtual bool indexFile(PMEntry_Linux &curEntry);




        //todo: Build a library dependency graph

        /**
         * /proc/{pid}/maps only tells the actual loading address and permission of a segment.
         * However, relocation table entries are relative to a base address
         * To get the real address we have to parse it seperately.
         */
        virtual void parseDLPhdr();

        friend int dlCallback(struct dl_phdr_info *info, size_t size, void *data);

    };


    class PmParserC_Linux : public PmParser_Linux {
    public:

        PmParserC_Linux(int procID = -1);

        ~PmParserC_Linux();

        int findExecNameByAddr(void *addr) override;

    protected:

        //C datastructure for PmParser_Linux::sortedSegments
        //Developers should fill these variable after sortedSegments is built
        size_t sortedSegSizeC = 0;
        void **sortedStartAddrC = nullptr;
        void **sortedEndAddrC = nullptr;
        int *sortedSegIDC = nullptr;


        /**
         * Although STL data structures are easy to use.
         * We can't use them inside certain part of the hook handler (before function ID is resolved).
         * That's why we need to transfer existing datastructures to C array.
         * In this way we could keep elegant code and still make it work.
         *
         * This function is called at the end of parsing stage. It's main function is to convert STL datastructure to
         * C compatible structure.
        */
        void fillCDataStructure();


    };

};


#endif
#endif
