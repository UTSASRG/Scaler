
#include "util/tool/ProcInfoParser.h"

#ifdef __linux__

#include <sstream>
#include <cassert>
#include <exceptions/ScalerException.h>
#include <algorithm>

#define PROCMAPS_LINE_MAX_LENGTH  (PATH_MAX + 100)
namespace scaler {
    PmParser_Linux::PmParser_Linux(int pid) : procID(pid) {
        openPMMap();
        parsePMMap();
    }

    void PmParser_Linux::openPMMap() {
        std::stringstream ss;
        ss << "/proc/";
        if (procID >= 0)
            ss << procID;
        else
            ss << "self";
        ss << "/maps";

        file.open(ss.str(), std::ios_base::in);
        if (!file) {
            std::stringstream ss1;
            ss1 << "Cannot open " << ss.str();
            throwScalerException(ss1.str().c_str());
        }
    }

    void PmParser_Linux::parsePMMap() {
        assert(file.is_open());

        std::string addr1, addr2, perm, offset;
        if (file.is_open()) {
            std::string line;

            while (std::getline(file, line)) {
                PMEntry_Linux newEntry;

                std::stringstream ss;
                //Parse current line
                ss << line;
                //Put all address into addr1. Seperate them afterwards
                ss >> addr1 >> perm >> offset >> newEntry.dev >> newEntry.inode >> newEntry.pathName;
                if (curExecFileName == "")
                    //The first filename wuold always be the executable file
                    curExecFileName = newEntry.pathName;

                //Split address into starting address
                auto splitPoint = addr1.find('-');
                //Ending address
                addr2 = addr1.substr(splitPoint + 1, addr1.length());
                //Starting address
                addr1 = addr1.substr(0, splitPoint);

                //Convert addr1,addr2 to void* and put it into addrStart (They are hex value)
                sscanf(addr1.c_str(), "%p", &newEntry.addrStart);
                sscanf(addr2.c_str(), "%p", &newEntry.addrEnd);

                //Put offset into offset (They are 8-bit hex without 0x prefix)
                sscanf(offset.c_str(), "%08x", &newEntry.offset);

                newEntry.length = ElfW(Addr)(newEntry.addrEnd) - ElfW(Addr)(newEntry.addrStart);

                //Fill permission
                newEntry.isR = (perm[0] == 'r');
                newEntry.isW = (perm[1] == 'w');
                newEntry.isE = (perm[2] == 'x');
                newEntry.isP = (perm[3] == 'p');

                //Map pathname to PmEntry for easier lookup
                procMap[newEntry.pathName].emplace_back(newEntry);

                sortedSegments.emplace_back(newEntry);
            }
            file.close();
        }
        //Sort sortedSegments by starting address
        std::sort(sortedSegments.begin(), sortedSegments.end(), [](const PMEntry_Linux &lhs, const PMEntry_Linux &rhs) {
            return lhs.addrStart < rhs.addrStart;
        });

        //Store entries name and it's id for faster lookup
        auto procMapIter = procMap.begin();
        for (int i = 0; i < procMap.size(); ++i) {
            auto &fileName = procMapIter->first;
            fileIDMap[fileName] = i;
            idFileMap.emplace_back(fileName);
            ++procMapIter;
        }


    }

    PmParser_Linux::~PmParser_Linux() {
        if (file.is_open()) {
            file.close();
        }

    }

    void PmParser_Linux::printPM() {
        std::stringstream ss;
        ss << "/proc/";
        if (procID >= 0)
            ss << procID;
        else
            ss << "self";
        ss << "/maps";

        std::ifstream ifs(ss.str());
        if (ifs.is_open())
            std::cout << ifs.rdbuf() << std::endl;
    }

    size_t PmParser_Linux::findExecNameByAddr(void *addr) {
        //Since sortedSegments are sorted by starting address and all address range are not overlapping.
        //We could use binary search to lookup addr in this array.


        //Binary search segAddrFileMap
        long lo = 0, hi = procMap.size(), md;
        //[lo,hi) to prevent underflow
        while (lo < hi - 1) {
            md = (lo + hi) / 2;
            if (sortedSegments[md].addrStart > addr) {
                hi = md;
            } else {
                lo = md + 1;
            }
        }
        return lo;
    }

}

#endif

