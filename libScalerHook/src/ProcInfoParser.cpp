
#include "util/tool/ProcInfoParser.h"

#ifdef __linux__

#include <sstream>
#include <cassert>
#include <exceptions/ScalerException.h>
#include <algorithm>
#include <util/tool/FileTool.h>


#define PROCMAPS_LINE_MAX_LENGTH  (PATH_MAX + 100)
namespace scaler {

    PmParser_Linux::PmParser_Linux(int pid) : procID(pid) {
        openPMMap();
        parsePMMap();
        //todo: Parse used dll
        //parseDLPhdr();
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
            std::string lastPathname = "";
            while (std::getline(file, line)) {
                PMEntry_Linux newEntry;

                std::stringstream ss;
                //Parse current line
                ss << line;
                //Put all address into addr1. Seperate them afterward
                ss >> addr1 >> perm >> offset >> newEntry.dev >> newEntry.inode >> newEntry.pathName;

                if (curExecAbsolutePath == "") {
                    //The first filename would always be the executable file
                    curExecAbsolutePath = newEntry.pathName;
                    extractFileName_Linux(curExecAbsolutePath, curExecPath, curExecFileName);
                }


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


                if (fileIDMap.count(newEntry.pathName) == 0) {
                    //New File, add it to fileId idFile map. Fill it's starting address as base address
                    idFileMap.emplace_back(newEntry.pathName);
                    startAddrFileMap[(uint8_t *) newEntry.addrStart] = idFileMap.size() - 1;
                    fileIDMap[newEntry.pathName] = idFileMap.size() - 1;
                    fileBaseAddrMap[idFileMap.size() - 1] = (uint8_t *) (newEntry.addrStart);
                }

                //Map pathname to PmEntry for easier lookup
                procMap[newEntry.pathName].emplace_back(newEntry);

                sortedSegments.emplace_back(std::make_pair(fileIDMap.at(newEntry.pathName), newEntry));
            }
            file.close();
        }
        //Sort sortedSegments by starting address
        std::sort(sortedSegments.begin(), sortedSegments.end(),
                  [](const std::pair<size_t, PMEntry_Linux> &lhs, const std::pair<size_t, PMEntry_Linux> &rhs) {
                      return (ElfW(Addr)) lhs.second.addrStart < (ElfW(Addr)) rhs.second.addrStart;
                  });

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

    int PmParser_Linux::findExecNameByAddr(void *addr) {
        //Since sortedSegments are sorted by starting address and all address range are not overlapping.
        //We could use binary search to lookup addr in this array.

        //Binary search segAddrFileMap
        long lo = 0, hi = sortedSegments.size(), md;
        //[lo,hi) to prevent underflow
        while (lo < hi - 1) {
            md = (lo + hi) / 2;

            if (sortedSegments[md].second.addrStart > addr) {
                hi = md;
            } else {
                lo = md;
            }

        }

        if (sortedSegments[lo].second.addrStart <= addr && addr <= sortedSegments[lo].second.addrEnd) {
            return startAddrFileMap.at((uint8_t *) sortedSegments[lo].second.addrStart);
        } else {
            return -1;
        }
    }

    int dlCallback(struct dl_phdr_info *info, size_t size, void *data) {
        //todo: file not found error
        PmParser_Linux *_this = static_cast<PmParser_Linux *>(data);

        if (_this->fileIDMap.count(std::string(info->dlpi_name)) != 0) {
            size_t curFileId = _this->fileIDMap.at(std::string(info->dlpi_name));
            _this->linkedFileID.emplace_back(curFileId);
        } else {
            printf("%s not found in /self/proc/maps\n", info->dlpi_name);
        }
        return 0;
    }

    void PmParser_Linux::parseDLPhdr() {
        dl_iterate_phdr(dlCallback, this);
    }

    void PmParser_Linux::curExecName() {

    }

    uint8_t *PmParser_Linux::autoAddBaseAddr(uint8_t *curBaseAddr, size_t curFileiD, ElfW(Addr) targetAddr) {
        size_t idWithBaseAddr = findExecNameByAddr(curBaseAddr + targetAddr);
        size_t idWithoutBaseAddr = findExecNameByAddr((void *) targetAddr);
        if (idWithBaseAddr == curFileiD) {
            //Relative
            return curBaseAddr;
        } else if (idWithoutBaseAddr == curFileiD) {
            //Absolute
            return nullptr;
        } else {
            printf("Not found, id1=%zu, id2=%zu, curFileID=%zu\n", idWithBaseAddr, idWithoutBaseAddr, curFileiD);
            assert(false);
        }
    }

    /**
     * Read the memory from another process. This function WON'T free the returned memory data. Use with great caution.
     *
     * @param startAddr Strating address to copy
     * @param bytes How many bytes to copy starting from startAddr
     * @return The copied memory (will not automatically free memory). In case there's a failure, nullptr will be returned (Permission not sufficient)
     */
    void *PmParser_Linux::readProcMem(void *startAddr, size_t bytes) {

        //Since it's inter-process. We use ptrace to copy memory first.
        int mem_fd;
        std::stringstream ss;
        ss << "/proc/";
        if (procID >= 0)
            ss << procID;
        else
            ss << "self";
        ss << "/mem";
        void *readRlt = malloc(bytes);
        memset(readRlt, 0, bytes);
        long long readErr, writeErr, seekErr;
        // open the memory
        try {
            mem_fd = open(ss.str().c_str(), O_RDONLY);
            if (mem_fd < 0) {
                throwScalerException("Open process memory file failed");
            }
            if (lseek(mem_fd, (__off_t) startAddr, SEEK_SET) < 0) {
                throwScalerException("Seek process memory file failed");
            }
            if (read(mem_fd, readRlt, bytes) < 0) {
                throwScalerException("Read process memory file failed");
            }
        } catch (const ScalerException &e) {
            perror(e.what());
            return nullptr;
        }
        return readRlt;
    }

    PmParserC_Linux::PmParserC_Linux(int procID) : PmParser_Linux(procID) {
        //Now parsing is complete by super clalss. We need to convert the datastructure to C-compatible local variable.
        fillCDataStructure();
    }


    int PmParserC_Linux::findExecNameByAddr(void *addr) {
        //Since sortedSegments are sorted by starting address and all address range are not overlapping.
        //We could use binary search to lookup addr in this array.

        //Binary search segAddrFileMap
        long lo = 0, hi = sortedSegSizeC, md;
        //[lo,hi) to prevent underflow
        while (lo < hi - 1) {
            md = (lo + hi) / 2;

            if (sortedStartAddrC[md] > addr) {
                hi = md;
            } else {
                lo = md;
            }

        }

        if (sortedStartAddrC[lo] <= addr && addr <= sortedEndAddrC[lo]) {
            return sortedSegIDC[lo];
        } else {
            return -1;
        }
    }


    PmParserC_Linux::~PmParserC_Linux() {
        delete[] sortedStartAddrC;
        delete[] sortedEndAddrC;
        delete[] sortedSegIDC;
    }

    void PmParserC_Linux::fillCDataStructure() {

        //Now sortedSegments is built. We'll copy it's content to
        sortedSegSizeC = sortedSegments.size();
        sortedStartAddrC = new void *[sortedSegSizeC];
        sortedEndAddrC = new void *[sortedSegSizeC];
        sortedSegIDC = new int[sortedSegSizeC];

        //Copy contents
        for (int i = 0; i < sortedSegSizeC; ++i) {
            sortedStartAddrC[i] = sortedSegments[i].second.addrStart;
            sortedEndAddrC[i] = sortedSegments[i].second.addrEnd;
            sortedSegIDC[i] = fileIDMap.at(sortedSegments[i].second.pathName);
        }

    }

    void PmParserC_Linux::openPMMap() {
        PmParser_Linux::openPMMap();
    }


}

#endif

