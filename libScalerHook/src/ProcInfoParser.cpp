
#include "util/tool/ProcInfoParser.h"

#ifdef __linux__

#include <sstream>
#include <cassert>
#include <exceptions/ScalerException.h>
#include <algorithm>
#include <util/tool/FileTool.h>
#include <exceptions/ErrCode.h>
#include <util/hook/hook.hh>


#define PROCMAPS_LINE_MAX_LENGTH  (PATH_MAX + 100)
namespace scaler {

    PmParser_Linux::PmParser_Linux(int pid) : procID(pid) {
        parsePMMap();
        //todo: Parse used dll
        //parseDLPhdr();
    }

    bool PmParser_Linux::parsePMMap() {
        fileIDMap.clear();
        fileBaseAddrMap.clear();
        startAddrFileMap.clear();
        idFileMap.clear();
        linkedFileID.clear();
        executableSegments.clear();
        readableSegments.clear();


        std::ifstream file;
        if (!openPMMap(file)) {
            return false;
        }

        std::string addr1, addr2, perm, offset;
        std::string line;
        while (std::getline(file, line)) {
            PMEntry_Linux newEntry;

            std::stringstream ss;
            //Parse current line
            ss << line;
            //Put all address into addr1. Seperate them afterward
            ss >> addr1 >> perm >> offset >> newEntry.dev >> newEntry.inode >> newEntry.pathName;

            if (curExecAbsolutePath.empty()) {
                //The first line would always be the executable file. Fill it in the data srtucture for future use
                curExecAbsolutePath = newEntry.pathName;
                if (!extractFileName_Linux(curExecAbsolutePath, curExecPath, curExecFileName)) {
                    return false;
                }
            }

            if (!parseAddrStr(newEntry, addr1)) {
                return false;
            }

            if (!parseOffsetStr(newEntry, offset)) {
                return false;
            }

            //Fill permission
            if (!parsePermStr(newEntry, perm)) {
                return false;
            }

            //Fill idFileMap, startAddrFileMap, fileIDMap, fileBaseAddrMap, sortedSegments
            if (!indexFile(newEntry)) {
                return false;
            }
        }
        file.close();

        //Sort sortedSegments by starting address
        std::sort(sortedSegments.begin(), sortedSegments.end(),
                  [](const std::pair<FileID, PMEntry_Linux> &lhs, const std::pair<FileID, PMEntry_Linux> &rhs) {
                      return (ElfW(Addr)) lhs.second.addrStart < (ElfW(Addr)) rhs.second.addrStart;
                  });

    }

    bool PmParser_Linux::openPMMap(std::ifstream &file) const {
        std::stringstream ss;
        ss << "/proc/";
        if (procID >= 0)
            ss << procID;
        else
            ss << "self";
        ss << "/maps";

        file.open(ss.str(), std::ios_base::in);
        if (file.fail()) {
            ERR_LOGS("Cannot open %s", ss.str().c_str());
            return false;
        }
        return true;
    }

    PmParser_Linux::~PmParser_Linux() {
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
            return 0;
        }
    }

    int dlCallback(struct dl_phdr_info *info, size_t size, void *data) {
        //todo: file not found error
        PmParser_Linux *_this = static_cast<PmParser_Linux *>(data);

        if (_this->fileIDMap.count(std::string(info->dlpi_name)) != 0) {
            FileID curFileId = _this->fileIDMap.at(std::string(info->dlpi_name));
            _this->linkedFileID.emplace_back(curFileId);
        } else {
            ERR_LOGS("%s not found in /self/proc/maps", info->dlpi_name);
            return 0;
        }
        //todo: double check ld manual return value.
        return 1;
    }

    void PmParser_Linux::parseDLPhdr() {
        dl_iterate_phdr(dlCallback, this);
    }

    uint8_t *PmParser_Linux::autoAddBaseAddr(uint8_t *curBaseAddr, FileID curFileiD, ElfW(Addr) targetAddr) {
        ssize_t idWithBaseAddr = findExecNameByAddr(curBaseAddr + targetAddr);
        ssize_t idWithoutBaseAddr = findExecNameByAddr((void *) targetAddr);
        if (idWithBaseAddr == curFileiD) {
            //Relative
            return curBaseAddr;
        } else if (idWithoutBaseAddr == curFileiD) {
            //Absolute
            return nullptr;
        } else {
            ERR_LOGS("Not found, id1=%zu, id2=%zu, curFileID=%zu\n", idWithBaseAddr, idWithoutBaseAddr, curFileiD);
            return nullptr;
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
        if (!readRlt) {
            ERR_LOG("Cannot allocate memory for bytes");
            return nullptr;
        }
        if (!memset(readRlt, 0, bytes)) {
            ERR_LOG("Cannot memset for bytes");
            return nullptr;
        }
        long long readErr, writeErr, seekErr;
        // open the memory
        mem_fd = open(ss.str().c_str(), O_RDONLY);
        if (!mem_fd) {
            free(readRlt);
            close(mem_fd);
            ERR_LOGS("Open process memory file %s failed", ss.str().c_str());
            return nullptr;
        }

        if (!lseek(mem_fd, (__off_t) startAddr, SEEK_SET)) {
            free(readRlt);
            close(mem_fd);
            ERR_LOGS("Seek process memory file %s failed", ss.str().c_str());
            return nullptr;
        }
        if (!read(mem_fd, readRlt, bytes)) {
            free(readRlt);
            close(mem_fd);
            ERR_LOGS("Read process memory file %s at location %p failed", ss.str().c_str(), startAddr);
            return nullptr;
        }

        close(mem_fd);
        return readRlt;
    }

    bool PmParser_Linux::parseAddrStr(PMEntry_Linux &curEntry, const std::string &addrStr) {
        //Split address into starting address
        auto splitPoint = addrStr.find('-');
        if (splitPoint == std::string::npos) {
            ERR_LOG("/pros/{pid}/map address format wrong, cannot find split -");
            return false;
        }

        //Starting address
        std::string addr1 = addrStr.substr(0, splitPoint);
        //Ending address
        std::string addr2 = addrStr.substr(splitPoint + 1, addrStr.length());
        //Convert addr1,addr2 to void* and put it into addrStart (They are hex value)
        if (!sscanf(addr1.c_str(), "%p", &curEntry.addrStart)) {
            ERR_LOG("/pros/{pid}/map address format wrong");
            return false;
        }

        if (!sscanf(addr2.c_str(), "%p", &curEntry.addrEnd)) {
            ERR_LOG("/pros/{pid}/map address format wrong");
            return false;
        }
        curEntry.length = ElfW(Addr)(curEntry.addrEnd) - ElfW(Addr)(curEntry.addrStart);
        return true;
    }

    bool PmParser_Linux::parseOffsetStr(PMEntry_Linux &curEntry, const std::string &offsetStr) {
        //Put offset into offset (They are 8-bit hex without 0x prefix)
        if (!sscanf(offsetStr.c_str(), "%8lx", &curEntry.offset)) {
            ERR_LOG("/pros/{pid}/map offset format wrong");
            return false;
        }
        return true;
    }

    void PmParser_Linux::parsePermStr(PMEntry_Linux &curEntry, const std::string &permStr) {
        curEntry.isR = (permStr[0] == 'r');
        curEntry.isW = (permStr[1] == 'w');
        curEntry.isE = (permStr[2] == 'x');
        curEntry.isP = (permStr[3] == 'p');

        //Create such index for faster lookup
        if (curEntry.isE)
            executableSegments.emplace_back(curEntry);
        else if (curEntry.isR)
            readableSegments.emplace_back(curEntry);
    }


    void PmParser_Linux::indexFile(PMEntry_Linux &curEntry) {
        if (fileIDMap.count(curEntry.pathName) == 0) {
            //Only add if it is a new file. New File, add it to fileId idFile map. Fill it's starting address as base address
            idFileMap.emplace_back(curEntry.pathName);

            fileBaseAddrMap.emplace_back(std::make_pair<uint8_t *, uint8_t *>((uint8_t *) curEntry.addrStart, nullptr));

            fileIDMap[curEntry.pathName] = idFileMap.size() - 1;
        } else {
            FileID fileID = fileIDMap.at(curEntry.pathName);
            //Assume address is incremental
            fileBaseAddrMap[fileID].second = (uint8_t *) (curEntry.addrEnd);
        }

        //Map pathname to PmEntry for easier lookup
        procMap[curEntry.pathName].emplace_back(curEntry);
        startAddrFileMap[(uint8_t *) curEntry.addrStart] = fileIDMap[curEntry.pathName];
        sortedSegments.emplace_back(std::make_pair(fileIDMap.at(curEntry.pathName), curEntry));
    }

    bool PmParser_Linux::addrInApplication(void *addr) {
        auto &appPmEmtry = sortedSegments[0].second;
        return ((appPmEmtry.addrStart <= addr) && (addr <= appPmEmtry.addrEnd));
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

}

#endif

