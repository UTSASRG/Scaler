
#include "util/tool/ProcInfoParser.h"

#ifdef __linux__

#include <sstream>
#include <cassert>
#include <exceptions/ScalerException.h>
#include <algorithm>
#include <util/tool/FileTool.h>
#include <util/tool/StringTool.h>
#include <set>
#include <climits>


#define PROCMAPS_LINE_MAX_LENGTH  (PATH_MAX + 100)
namespace scaler {


    PmParser::PmParser() : pmEntryArray(70) {
    }

    bool PmParser::parsePMMap() {
        pmEntryArray.clear();


        const char *procIdStr = "/proc/self/maps";
        FILE *procFile = fopen(procIdStr, "rb");
        if (!procFile) {
            ERR_LOGS("Cannot open %s because: %s", procIdStr, strerror(errno));
            return false;
        }
        FILE *execNameFile = fopen("fileName.txt", "w");
        if (!execNameFile) {
            ERR_LOGS("Cannot open %s because: %s", "fileName.txt", strerror(errno));
            return false;
        }

        std::string addr1, addr2, perm, offset;

        char procMapLine[512];
        char prevPathname[PATH_MAX];
        ssize_t prevPathNameLen = -1;
        char pathName[PATH_MAX];
        char permStr[8];
        ssize_t fileId = -1;

        while (fgets(procMapLine, sizeof(procMapLine), procFile)) {
#ifndef NDEBUG
            //Make sure the buffer is enough
            size_t len = strnlen(procMapLine, sizeof(procMapLine));
            if (len != 0 && procMapLine[len - 1] != '\n') {
                fatalErrorS("Line in /proc/{pid}/map exceeded buffer size %lu. Please adjust procMapLine size",
                            sizeof(procMapLine));
                return false;
            }
#endif
            PMEntry *newEntry = pmEntryArray.pushBack();

            int scanfReadNum = sscanf(procMapLine, "%p-%p %8s %*s %*s %*s %s", &newEntry->addrStart, &newEntry->addrEnd,
                                      permStr, pathName);

            std::string dirName;
            std::string fileName;
            extractFileName(pathName, dirName, fileName);

            //todo: debug
//            if(strncmp(fileName.c_str(),"ScalerHook-demoapps-FuncCall",fileName.size())!=0){
//                pmEntryArray.popBack();
//                continue;
//            }




            //Check scanf succeeded or not
            if (scanfReadNum == 3) {
                //DBG_LOGS("No file name, ignore line: %s", procMapLine);
                pmEntryArray.popBack();
                continue;
            } else if (pathName[0] == '[') {
                //DBG_LOGS("Illegal filename, ignore line:%s", procMapLine);
                pmEntryArray.popBack();
                continue;
            } else if (scanfReadNum != 4) {
                fatalErrorS("Parsing line %s failed, if this line looks normal check limits?", procMapLine);
            }

            if (strStartsWith(fileName, "libScalerHook")) {
                //DBG_LOGS("Scaler library skip");
                pmEntryArray.popBack();
                continue;
            }
            if (strStartsWith(fileName, "ld-")) {
                //DBG_LOGS("Scaler library skip");
                pmEntryArray.popBack();
                continue;
            }

            //Parse permission
            if (permStr[0] == 'r') {
                newEntry->setR();
            }
            if (permStr[1] == 'w') {
                newEntry->setW();
            }
            if (permStr[2] == 'x') {
                newEntry->setE();
            }
            if (permStr[3] == 'p') {
                newEntry->setP();
            }
            //Update filepathname
            //Check whether this is a new file
            ssize_t pathNameLen = strnlen(pathName, sizeof(pathName));
            if (pathNameLen != prevPathNameLen || strncmp(prevPathname, pathName, pathNameLen) != 0) {
                //Discovered a new file
                prevPathNameLen = pathNameLen;
                memcpy(prevPathname, pathName, pathNameLen + 1);
                //Write filename to disk

                fileNameArr.emplace_back(pathName);
                ++fileId;
                //DBG_LOGS("%s\n", pathName);
#ifndef NDEBUG
                //Check fileId overflow
                if (fileId > SHRT_MAX) {
                    fatalError("File id is larger than short maximum.")
                    exit(-1);
                }
#endif

            }
            newEntry->fileId = fileId;

        }

        fclose(procFile);
        fclose(execNameFile);

#ifndef NDEBUG
        void *curAddr = nullptr;
        for (int i = 0; i < pmEntryArray.getSize(); ++i) {
            if (pmEntryArray[i].addrStart < curAddr) {
                fatalError("/proc/{pid}/maps address is assumed to be always increasing.")
                exit(-1);
            }
            curAddr = pmEntryArray[i].addrStart;
        }
#endif
        return true;
    }


    PmParser::~PmParser() {
    }

    void PmParser::printPM() {
        std::stringstream ss;
        ss << "/proc/self/maps";

        std::ifstream ifs(ss.str());
        if (ifs.is_open())
            std::cout << ifs.rdbuf() << std::endl;
    }

    ssize_t PmParser::findExecNameByAddr(void *addr) {
        //Since sortedSegments are sorted by starting address and all address range are not overlapping.
        //We could use binary search to lookup addr in this array.

        //Binary search impl segAddrFileMap
        ssize_t lo = 0;
        ssize_t hi = pmEntryArray.getSize();
        ssize_t md;
        bool found = false;
        while (lo != hi) {
            md = (lo + hi) / 2;
            if (pmEntryArray[md].addrStart < addr) {
                //printf("hi(%d) = md(%d) - 1=(%d)\n", hi, md, md - 1);
                lo = md + 1;
            } else if (pmEntryArray[md].addrStart > addr) {
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


        //It is possible that the address falls within the range of last entry. We need to check this scenario

        if (lo == -1) {
            fatalErrorS(
                    "Cannot find addr %p in pmMap. The address is lower than the lowest address if /proc/{pid}/maps.",
                    addr);
            exit(-1);
        } else if (lo == pmEntryArray.getSize()) {
            //Address is within range
            lo = pmEntryArray.getSize() - 1;
        }

        //Check if it's end address is indeed in this entry. If not, it is because the caller is not in procinfomapper
        // (Maybe skipped, in this case return an id that is larger than the largest function addr)
        if (addr > pmEntryArray[lo].addrEnd) {
            return fileNameArr.size();
        }

        return pmEntryArray[lo].fileId;
    }


    uint8_t *PmParser::autoAddBaseAddr(uint8_t *curBaseAddr, FileID curFileiD, ElfW(Addr) targetAddr) {
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
            ERR_LOGS("Not found, id1=%zu, id2=%zu, curFileID=%zd\n", idWithBaseAddr, idWithoutBaseAddr, curFileiD);
            return nullptr;
        }
    }

}

#endif

