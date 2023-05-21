
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
#include <utility>


#define PROCMAPS_LINE_MAX_LENGTH  (PATH_MAX + 100)
namespace scaler {


    PmParser::PmParser(std::string saveFolderName, std::string customProcFileName) : folderName(saveFolderName),
                                                                                     customProcFileName(
                                                                                             customProcFileName),
                                                                                     pmEntryArray(70) {
    }


    bool PmParser::parsePMMap(ssize_t loadingId) {
        if (previousLoaidngId != loadingId - 1) {
            fatalErrorS("loaidngID should be always incremental Previous:%zd Current:%zd", previousLoaidngId,
                        loadingId);
        }
        previousLoaidngId = loadingId;

        FILE *procFile = openProcFile();
        if (!procFile)
            return false;

        std::string addr1, addr2, perm, offset;

        //Save the filename of this loading
        char procMapLine[512];
        char permStr[9];
        uint8_t *addrStart;
        uint8_t *addrEnd;
        std::stringstream ss;
        ss << folderName << "/" << loadingId << "fileName.txt";
        FILE *fileNameStrTbl = fopen(ss.str().c_str(), "w");
        if (!fileNameStrTbl) {
            fatalErrorS("Cannot open %s", ss.str().c_str());
        }

        fprintf(fileNameStrTbl, "%s,%s\n", "fileId", "pathName");

        while (fgets(procMapLine, sizeof(procMapLine), procFile)) {
#ifndef NDEBUG
            //Make sure the buffer is enough
            size_t len = strnlen(procMapLine, sizeof(procMapLine));
            if (len != 0 && procMapLine[len] != '\0') {
                fatalErrorS("Line %s in /proc/{pid}/map exceeded buffer size %lu. Please adjust procMapLine size",
                            procMapLine, sizeof(procMapLine));
                return false;
            }
#endif
            //INFO_LOGS("New line: %s",procMapLine);

            char pathName[PATH_MAX] = "";
            //Read pmEntry line
            int scanfReadNum = sscanf(procMapLine, "%p-%p %8s %*s %*s %*s %s", &addrStart, &addrEnd, permStr, pathName);

            ssize_t pathNameLen = strlen(pathName);

            //Find if there is a match based on address search
            ssize_t lo;
            bool found = false;
            findPmEntryIdByAddr(addrStart, lo, found);

            PMEntry *newPmEntry = nullptr;
            if (found) {
                PMEntry &pmEntry = pmEntryArray[lo];
                FileEntry &fileEntry = fileEntryArray[pmEntry.fileId];
                fileEntry.loadingId = loadingId;

                if (pmEntry.addrEnd == addrEnd &&
                    strncmp(stringTable.c_str() + fileEntry.pathNameStartIndex, pathName,
                            fileEntry.getPathNameLength()) == 0) {
                    //Exactly the same entry (Ignore permission and other attrs). Replace permission fields just in case.
                    //Update loading id
                    pmEntry.loadingId = loadingId;
                    pmEntry.setPermBits(permStr);
                    continue;
                } else {
                    //Same starting address, but different end address/fileName. Replace entry, and remove linkage to the original fileEntry
                    //INFO_LOG("Same starting address, but different end address/fileName. Replace entry, and remove linkage to the original fileEntry");
                    newPmEntry = &pmEntry;
//                    newPmEntry->loadingId=loadingId;//Update the loading id
                    assert(fileEntry.pmEntryNumbers > 0);
                    fileEntry.pmEntryNumbers -= 1; //Remove linkage to the previous file entry
                }
            } else {
                //Not found, create a new PmEntry
                newPmEntry = pmEntryArray.insertAt(lo);
//                newPmEntry->loadingId = loadingId;//Update the loading id
            }
            newPmEntry->loadingId = loadingId;//Update the loading id
            newPmEntry->addrStart = addrStart;
            newPmEntry->addrEnd = addrEnd;
            newPmEntry->fileId = -1;//Allocate and set later
            newPmEntry->setPermBits(permStr);

            //Check if we need to allocate a new fileId or not by comparing with previous pmEntry's fileName.
            //Linearly search for the same file

            if (matchWithPreviousFileId(lo, loadingId, pathName, pathNameLen, newPmEntry)) {
                //New
                continue;
            }

            createFileEntry(newPmEntry, loadingId, pathName, scanfReadNum);

        }

        //Delete deleted pmEntries
        for (int i = pmEntryArray.getSize() - 1; i >= 0; --i) {
            //Remove all non-updated PLT entries (which means entries no longer exists)
            assert(abs(loadingId - pmEntryArray[i].loadingId) <= 1);

            if (pmEntryArray[i].loadingId < loadingId) {
                //Currently we do not need to return this
                fileEntryArray[pmEntryArray[i].fileId].pmEntryNumbers -= 1;//Unlink pmEntry
                pmEntryArray.erase(i);
            }
        }


        fclose(fileNameStrTbl);
        fclose(procFile);

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

    ssize_t PmParser::findFileIdByAddr(void *addr) {
        bool found = false;
        ssize_t pmEntryId;
        findPmEntryIdByAddr(addr, pmEntryId, found);
        //Since we only search PLT, it is impossible to hit the memory segment boundary.
        assert(found == false && 0 <= pmEntryId - 1 && pmEntryId - 1 < pmEntryArray.getSize());
        return pmEntryArray[pmEntryId].fileId;
    }

    void PmParser::findPmEntryIdByAddr(void *addr, ssize_t &lo, bool &found) {
        //Since sortedSegments are sorted by starting address and all address range are not overlapping.
        //We could use binary search to lookup addr in this array.

        //Binary search impl segAddrFileMap
        lo = 0;
        ssize_t md = 0, hi = pmEntryArray.getSize() - 1;
        found = true;
        while (lo <= hi) {
            md = lo + (hi - lo) / 2;
            if (pmEntryArray[md].addrStart < addr) {
                //printf("hi(%d) = md(%d) - 1=(%d)\n", hi, md, md - 1);
                lo = md + 1;
            } else if (pmEntryArray[md].addrStart > addr) {
                //printf("lo(%d) = md(%d) + 1=(%d)\n", lo, md, md + 1);
                hi = md - 1;
            } else {
                //Find left bound, although this should be impossible in this case
                hi = md - 1;
            }
        }
        if (lo >= pmEntryArray.getSize() || pmEntryArray[lo].addrStart != addr) {
            found = false;
        }
    }

    uint8_t *PmParser::autoAddBaseAddr(uint8_t *curBaseAddr, FileID curFileiD, ElfW(Addr) targetAddr) {
        ssize_t idWithBaseAddr = findFileIdByAddr(curBaseAddr + targetAddr);
        ssize_t idWithoutBaseAddr = findFileIdByAddr((void *) targetAddr);

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

    FILE *PmParser::openProcFile() {
        FILE *procFile = nullptr;
        if (customProcFileName.empty()) {
            const char *procIdStr = "/proc/self/maps";
            //Using test file rather than real proc filesystem
            procFile = fopen(procIdStr, "rb");
            if (!procFile) {
                ERR_LOGS("Cannot open /proc/self/maps because: %s", strerror(errno));
                return nullptr;
            }
        } else {
            procFile = fopen(customProcFileName.c_str(), "rb");
            if (!procFile) {
                ERR_LOGS("Cannot open %s because: %s", customProcFileName.c_str(), strerror(errno));
                return nullptr;
            }
        }
        return procFile;
    }

    bool PmParser::matchWithPreviousFileId(ssize_t startingId, ssize_t curLoadingId, char *pathName,
                                           ssize_t pathNameLen, PMEntry *newPmEntry) {
        bool hasPreviousFileNameMatch = false;
        for (int i = startingId - 1; i >= 0; --i) {
            if (curLoadingId - fileEntryArray[pmEntryArray[i].fileId].loadingId <= 1
                && fileEntryArray[pmEntryArray[i].fileId].getPathNameLength() == pathNameLen
                && strncmp(stringTable.c_str() + fileEntryArray[pmEntryArray[i].fileId].pathNameStartIndex,
                           pathName, pathNameLen) == 0) {
                //Previous filename matches with current file name, no need to create file entry
                newPmEntry->fileId = pmEntryArray[i].fileId;
                fileEntryArray[pmEntryArray[i].fileId].loadingId = curLoadingId;
                fileEntryArray[pmEntryArray[i].fileId].pmEntryNumbers += 1;
                hasPreviousFileNameMatch = true;
                break;
            }
        }
        return hasPreviousFileNameMatch;
    }

    void PmParser::createFileEntry(PMEntry *newPmEntry, ssize_t loadingId, char *pathName, ssize_t scanfReadNum) {
        ssize_t newFileId = fileEntryArray.getSize();
        FileEntry *newFileEntry = fileEntryArray.pushBack(); //We should not use insertAt because fileId is hard-coded into dynamically generated assembly instructions.
        newPmEntry->fileId = newFileId;
        newFileEntry->loadingId = loadingId;
        newFileEntry->creationLoadingId = loadingId;
        newFileEntry->pmEntryNumbers += 1;
        newFileEntry->pathNameStartIndex = stringTable.size();
        stringTable.append(pathName);
        newFileEntry->pathNameEndIndex = stringTable.size();
        newFileEntry->valid = false;//Decide later

        //Check the validity of fileEntry
        std::string dirName;
        std::string fileName;
        extractFileName(pathName, dirName, fileName);

        //Check scanf succeeded or not
        if (scanfReadNum == 3) {
            //DBG_LOGS("No file name, do not create file entry: %s", procMapLine);
            newFileEntry->valid = false;
        } else if (pathName[0] == '[') {
            //DBG_LOGS("Illegal filename, do not create file entry:%s", procMapLine);
            newFileEntry->valid = false;
        } else if (scanfReadNum != 4) {
            newFileEntry->valid = false;
            fatalError("Parsing line failed, if this line looks normal, check limits.");
        } else if (strStartsWith(fileName, "libScalerHook")) {
            //DBG_LOG("Do not create file entry for Scaler library");
            newFileEntry->valid = false;
        } else if (strStartsWith(fileName, "ld-")) {
            //DBG_LOG("Do not hook ld.so library");
            newFileEntry->valid = false;
        }
    }


}

#endif

