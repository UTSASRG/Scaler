
#include "util/tool/ProcInfoParser.h"

#ifdef __linux__

#include <sstream>
#include <cassert>
#include <exceptions/ScalerException.h>
#include <algorithm>
#include <util/tool/FileTool.h>
#include <exceptions/ErrCode.h>
#include <util/hook/hook.hh>
#include <util/tool/StringTool.h>
#include <set>


#define PROCMAPS_LINE_MAX_LENGTH  (PATH_MAX + 100)
namespace scaler {

    PmParser_Linux::PmParser_Linux(int pid) : procID(pid) {
        if (!parsePMMap()) {
            fatalError("Cannot parse mp. Check format?")
        }
        //todo: Parse used dll
    }

    bool PmParser_Linux::parsePMMap() {
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

#ifndef NODEBUG
        void *curAddr = nullptr;
        for (auto &i : pmEntryVector) {
            if (i.addrStart < curAddr) {
                fatalError("/proc/{pid}/maps address is assumed to be always increasing.")
                exit(-1);
            }
            curAddr = i.addrStart;
        }

        //Sort sortedSegments by starting address
//        std::sort(pmEntryVector.begin(), pmEntryVector.end(),
//                  [](const PMEntry_Linux &lhs, const PMEntry_Linux &rhs) {
//                      return (ElfW(Addr)) lhs.addrStart < (ElfW(Addr)) rhs.addrStart;
//                  });
#endif
        return true;
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

    ssize_t PmParser_Linux::findExecNameByAddr(void *addr) {
        //Since sortedSegments are sorted by starting address and all address range are not overlapping.
        //We could use binary search to lookup addr in this array.

        //Binary search impl segAddrFileMap
        ssize_t lo = 0;
        ssize_t hi = pmEntryVector.size();
        ssize_t md;
        bool found = false;
        while (lo != hi) {
            md = (lo + hi) / 2;
            if (pmEntryVector[md].addrStart < addr) {
                //printf("hi(%d) = md(%d) - 1=(%d)\n", hi, md, md - 1);
                lo = md + 1;
            } else if (pmEntryVector[md].addrStart > addr) {
                //printf("lo(%d) = md(%d) + 1=(%d)\n", lo, md, md + 1);
                hi = md;
            } else {
                //printf("lo = md =%d\n", md);
                lo = md;
                found = true;
                break;
            }
        }
        if (!found && lo == 0) {
            lo = -1;
        }

        if (lo == -1) {
            fatalErrorS(
                    "Cannot find addr %p in pmMap. The address is lower than the lowest address if /proc/{pid}/maps.",
                    addr);
            exit(-1);
        } else if (lo == pmEntryVector.size()) {
            //Check if it's end address is covered in the last entry
            if (addr > pmEntryVector[pmEntryVector.size() - 1].addrEnd) {
                fatalErrorS(
                        "Cannot find addr %p in pmMap. The address is higher than the highest address if /proc/{pid}/maps.",
                        addr);
                exit(-1);
            }
            //Address is within range
            lo = pmEntryVector.size() - 1;
        }

        return lo;
    }


    uint8_t *PmParser_Linux::autoAddBaseAddr(uint8_t *curBaseAddr, FileID curFileiD, ElfW(Addr) targetAddr) {
        ssize_t idWithBaseAddr = findExecNameByAddr(curBaseAddr + targetAddr);
        ssize_t idWithoutBaseAddr = findExecNameByAddr((void *) targetAddr);

        DBG_LOGS("idWithBaseAddr=%zd, idWithoutBaseAddr=%zd %p", idWithBaseAddr, idWithoutBaseAddr, curBaseAddr);

        if (idWithoutBaseAddr == curFileiD) {
            //Absolute
            return nullptr;
        } else if (idWithBaseAddr == curFileiD) {
            //Relative
            return curBaseAddr;
        } else {
            ERR_LOGS("Not found, id1=%zu, id2=%zu, curFileID=%hd\n", idWithBaseAddr, idWithoutBaseAddr, curFileiD);
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
        if (!sscanf(offsetStr.c_str(), "%8llx", &curEntry.offset)) {
            ERR_LOG("/pros/{pid}/map offset format wrong");
            return false;
        }
        return true;
    }

    bool PmParser_Linux::parsePermStr(PMEntry_Linux &curEntry, ssize_t curEntryIndex, const std::string &permStr) {
        curEntry.isR = (permStr[0] == 'r');
        curEntry.isW = (permStr[1] == 'w');
        curEntry.isE = (permStr[2] == 'x');
        curEntry.isP = (permStr[3] == 'p');
        return true;
    }

}

#endif

