
#include "util/tool/PMParser.h"
#include <sstream>
#include <cassert>
#include <exceptions/ScalerException.h>

#define PROCMAPS_LINE_MAX_LENGTH  (PATH_MAX + 100)

namespace scaler {
    PMParser::PMParser(int pid) : procID(pid) {
        openPMMap();
    }

    void PMParser::openPMMap() {
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

    void PMParser::parsePMMap() {
        assert(file.is_open());

        std::string addr1, addr2, perm, offset;
        if (file.is_open()) {
            std::string line;

            while (std::getline(file, line)) {
                PMEntry newEntry;
                // using printf() in all tests for consistency
                std::stringstream ss;
                ss << line;
                ss >> addr1 >> perm >> offset >> newEntry.dev >> newEntry.inode >> newEntry.pathName;
                if (curExecFileName == "")
                    curExecFileName = newEntry.pathName;
                auto splitPoint = addr1.find('-');
                addr2 = addr1.substr(splitPoint + 1, addr1.length());
                addr1 = addr1.substr(0, splitPoint);

                sscanf(addr1.c_str(), "%p", &newEntry.addrStart);
                sscanf(addr2.c_str(), "%p", &newEntry.addrEnd);
                sscanf(offset.c_str(), "%08x", &newEntry.offset);

                newEntry.length = (unsigned long) (newEntry.addrEnd) - (unsigned long) (newEntry.addrStart);

                //perm
                newEntry.isR = (perm[0] == 'r');
                newEntry.isW = (perm[1] == 'w');
                newEntry.isE = (perm[2] == 'x');
                newEntry.isP = (perm[3] == 'p');

                procMap[newEntry.pathName].push_back(newEntry);

                ss.str("");
                ss.clear();
            }
            file.close();
        }

    }

    PMParser::~PMParser() {
        if (file.is_open()) {
            file.close();
        }

    }

    void PMParser::printPM() {
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

}
