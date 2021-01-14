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

namespace scaler {
    class PMEntry : public Object {
    public:
        void *addrStart;    // start address of the area
        void *addrEnd;    // end address
        unsigned long length; // size of the range
        bool isR;            // permission
        bool isW;
        bool isE;
        bool isP;

        long offset;    // offset
        std::string dev;  // device major:minor
        int inode;        // inode of the file that backs the area

        std::string pathName; //Path name to that executable

        friend std::ostream &operator<<(std::ostream &os, const PMEntry &dt) {

            std::ios_base::fmtflags oldState(os.flags());

            std::stringstream ss;
            ss << (dt.isR ? 'r' : '-');
            ss << (dt.isW ? 'w' : '-');
            ss << (dt.isE ? 'x' : '-');
            ss << (dt.isP ? 'p' : '-');

            uint64_t addrStart = reinterpret_cast<uint64_t>(dt.addrStart);
            uint64_t addrEnd = reinterpret_cast<uint64_t>(dt.addrEnd);

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
     * This class was a helper tool to parse /proc/self/map
     * Current implementation uses STL API and may not the most efficient way. But it's fine for initialization and the code looks cleaner.
     */
    class PMParser : public Object {
    public:
        std::map<std::string, std::vector<PMEntry>> procMap;
        std::string curExecFileName;

        PMParser(int procID = -1);

        void parsePMMap();

        void printPM();

        ~PMParser() override;

    private:
        int procID;
        std::ifstream file;


        void openPMMap();

    };

}
#endif
