
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
        pthread_rwlock_init(&rwlock, NULL);
    }


    bool PmParser::parsePMMapUnSafe(ssize_t loadingId) {
        if (previousLoaidngId != loadingId - 1) {
            fatalErrorS("loaidngID should be always incremental Previous:%zd Current:%zd", previousLoaidngId,
                        loadingId);
        }
        previousLoaidngId = loadingId;

        FILE *procFile = openProcFileUnSafe();
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

        fprintf(fileNameStrTbl, "%s,%s\n", "globalFileId", "pathName");


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
            char pathName[PATH_MAX] = "";
            //Read pmEntry line
            int scanfReadNum = sscanf(procMapLine, "%p-%p %8s %*s %*s %*s %s", &addrStart, &addrEnd, permStr, pathName);

            ssize_t pathNameLen = strlen(pathName);

            //Find if there is a match based on address search
            ssize_t lo;
            bool found = false;
            findPmEntryIdByAddrUnSafe(addrStart, lo, found);
            PMEntry *newPmEntry = nullptr;
            ssize_t fileIdSearchStartingPoint=0;
            if (found) {
                PMEntry &pmEntry = pmEntryArray[lo];
                FileEntry &fileEntry = fileEntryArray[pmEntry.globalFileId];
                fileEntry.loadingId = loadingId;
                fileIdSearchStartingPoint=pmEntry.globalFileId;
                bool endAddressIsTheSame = (pmEntry.addrEnd == addrEnd);
                bool fileNameIsTheSame = (strncmp(&stringTable.get(fileEntry.pathNameStartIndex), pathName,
                                                  fileEntry.getPathNameLength()) == 0);

                if (endAddressIsTheSame && fileNameIsTheSame) {
                    //Exactly the same entry (Ignore permission and other attrs). Replace permission fields just in case.
                    //Update loading id
                    //INFO_LOG("Exactly the same entry");
                    pmEntry.loadingId = loadingId;
                    pmEntry.setPermBits(permStr);
                    continue;
                } else if (!endAddressIsTheSame && fileNameIsTheSame) {
                    //Only end address change, replace it and do not create file entry
                    pmEntry.addrEnd = addrEnd;
                    continue;
                } else {
                    //FileName is not the same, endAddress may or may not be the same.
                    //Replace end address, and remove linkage to the original fileEntry
                    //INFO_LOG("Same starting address, but different end address/fileName. Replace entry, and remove linkage to the original fileEntry");
                    newPmEntry = &pmEntry;
                    assert(fileEntry.pmEntryNumbers > 0);
                    fileEntry.pmEntryNumbers -= 1; //Remove linkage to the previous file entry
                    //INFO_LOG("Same starting address, but different end address/fileName.");
                    //INFO_LOGS("%p %p %s %s", pmEntry.addrEnd, addrEnd, &stringTable.get(fileEntry.pathNameStartIndex),
                    //          pathName);
                }
            } else {
                fileIdSearchStartingPoint=fileEntryArray.getSize()-1;
                //Not found, create a new PmEntry
                //INFO_LOG("Not found, create a new PmEntry");
                newPmEntry = pmEntryArray.insertAt(lo);
            }
            newPmEntry->creationLoadingId = loadingId;
            newPmEntry->loadingId = loadingId;//Update the loading id
            newPmEntry->addrStart = addrStart;
            newPmEntry->addrEnd = addrEnd;//Update end address
            newPmEntry->globalFileId = -1;//Allocate and set later
            newPmEntry->setPermBits(permStr);

            //Check if we need to allocate a new globalFileId or not by comparing with previous pmEntry's fileName.
            //Linearly search for the same file
            //INFO_LOGS("Try to match line: %s", procMapLine);
            if (matchWithPreviousFileIdUnSafe(loadingId, pathName, pathNameLen,
                                              newPmEntry)) {
                //New
                continue;
            }
            //INFO_LOGS("Create new file entry line: %s", procMapLine);
            //INFO_LOGS("Array Size: %s", procMapLine);
            createFileEntryUnSafe(newPmEntry, loadingId, pathName, pathNameLen, scanfReadNum);

        }

        //Delete deleted pmEntries
        rmDeletedPmEntriesUnsafe(loadingId);

        //Clear baseStartAddr
        updateFileBaseAddrUnsafe();

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

    void PmParser::updateFileBaseAddrUnsafe() {
        for (int i = 0; i < fileEntryArray.getSize(); ++i) {
            fileEntryArray[i].baseStartAddr = reinterpret_cast<uint8_t *>(UINTPTR_MAX);
            fileEntryArray[i].baseEndAddr = 0;
        }

        //Update baseStartAddr
        for (int i = 0; i < pmEntryArray.getSize(); ++i) {
            FileEntry &curFileEntry = fileEntryArray[pmEntryArray[i].globalFileId];
            curFileEntry.baseStartAddr = min(curFileEntry.baseStartAddr, pmEntryArray[i].addrStart);
            curFileEntry.baseEndAddr = max(curFileEntry.baseEndAddr, pmEntryArray[i].addrEnd);
        }
    }

    void PmParser::rmDeletedPmEntriesUnsafe(ssize_t loadingId) {
        for (int i = pmEntryArray.getSize() - 1; i >= 0; --i) {
            //Remove all non-updated PLT entries (which means entries no longer exists)
            assert(abs(loadingId - pmEntryArray[i].loadingId) <= 1);

            if (pmEntryArray[i].loadingId < loadingId) {
                //Currently we do not need to return this
                fileEntryArray[pmEntryArray[i].globalFileId].pmEntryNumbers -= 1;//Unlink pmEntry
                pmEntryArray.erase(i);
            }
        }
    }

    bool PmParser::parsePMMap(ssize_t loadingId) {
        pthread_rwlock_wrlock(&rwlock);
        bool rlt = parsePMMapUnSafe(loadingId);
        pthread_rwlock_unlock(&rwlock);
        return rlt;
    }


    PmParser::~PmParser() {
        if (pthread_rwlock_destroy(&rwlock) != 0) {
            fatalErrorS("Cannot destory pthread rwlock because: %s", strerror(errno));
        }
    }

    void PmParser::printPM() {
        std::stringstream ss;
        ss << "/proc/self/maps";

        std::ifstream ifs(ss.str());
        if (ifs.is_open())
            std::cout << ifs.rdbuf() << std::endl;
    }

    ssize_t PmParser::findFileIdByAddrUnSafe(void *addr) {
        bool found = false;
        ssize_t pmEntryId;
        findPmEntryIdByAddr(addr, pmEntryId, found);
        //Since we only search PLT, it is impossible to hit the memory segment boundary.
        assert(found == false && 0 <= pmEntryId - 1 && pmEntryId - 1 < pmEntryArray.getSize());
        ssize_t rlt = pmEntryArray[pmEntryId].globalFileId;
        return rlt;
    }

    ssize_t PmParser::findFileIdByAddr(void *addr) {
        pthread_rwlock_rdlock(&rwlock);
        ssize_t rlt = findFileIdByAddrUnSafe(addr);
        pthread_rwlock_unlock(&rwlock);
        return rlt;
    }

    void PmParser::findPmEntryIdByAddrUnSafe(void *addr, ssize_t &lo, bool &found) {
        //Since sortedSegments are sorted by starting address and all address range are not overlapping.
        //We could use binary search to lookup addr in this array.
        //Binary search impl segAddrFileMap
        lo = 0;
        ssize_t md = 0, hi = pmEntryArray.getSize() - 1;
        found = true;
        //INFO_LOGS("pmEntryArray.getSize()=%zd\n",pmEntryArray.getSize());

        while (lo <= hi) {
            md = lo + (hi - lo) / 2;
            assert(lo <= hi);
            //DBG_LOGS("lo=%zd, md=%zd,hi=%zd",lo,md,hi);
            //DBG_LOGS("pmEntryArray[%zd]",md);
            if (pmEntryArray[md].addrStart < addr) {
                //printf("hi(%zd) = md(%zd) - 1=(%zd)\n", hi, md, md - 1);
                lo = md + 1;
            } else if (pmEntryArray[md].addrStart > addr) {
                //printf("lo(%zd) = md(%zd) + 1=(%zd)\n", lo, md, md + 1);
                hi = md - 1;
            } else {
                //Find left bound, although this should be impossible in this case
                hi = md - 1;
            }
        }

        if (pmEntryArray.getSize() == 0) {
            found = false;
        } else if (lo >= pmEntryArray.getSize() || pmEntryArray[lo].addrStart != addr) {
            found = false;
            //INFO_LOGS("Not Found %zd %zd %p %p",lo,pmEntryArray.getSize(),pmEntryArray[lo].addrStart,addr);
        }
    }

    void PmParser::findPmEntryIdByAddr(void *addr, ssize_t &lo, bool &found) {
        pthread_rwlock_rdlock(&rwlock);
        findPmEntryIdByAddrUnSafe(addr, lo, found);
        pthread_rwlock_unlock(&rwlock);
    }

    uint8_t *PmParser::autoAddBaseAddrUnSafe(uint8_t *curBaseAddr, FileID curFileiD, ElfW(Addr) targetAddr) {
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

    uint8_t *PmParser::autoAddBaseAddr(uint8_t *curBaseAddr, FileID curFileID, ElfW(Addr) targetAddr) {
        pthread_rwlock_rdlock(&rwlock);
        uint8_t *rlt = autoAddBaseAddrUnSafe(curBaseAddr, curFileID, targetAddr);
        pthread_rwlock_unlock(&rwlock);
        return rlt;
    }

    FILE *PmParser::openProcFileUnSafe() {
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


    bool PmParser::matchWithPreviousFileIdUnSafe(ssize_t curLoadingId, char *pathName,
                                                 ssize_t pathNameLen, PMEntry *newPmEntry) {
        bool hasPreviousFileNameMatch = false;
        //Search forward
        for (ssize_t i = 0; i < this->fileEntryArray.getSize(); ++i) {
//            INFO_LOGS("Compare %s(%zd) with %s(%zd)", pathName, pathNameLen,
//                      &stringTable.get(fileEntryArray[pmEntryArray[i].globalFileId].pathNameStartIndex),
//                      fileEntryArray[pmEntryArray[i].globalFileId].getPathNameLength());
            if (curLoadingId - fileEntryArray[i].loadingId <= 1
                && fileEntryArray[i].pmEntryNumbers>0
                && fileEntryArray[i].getPathNameLength() == pathNameLen
                && strncmp(&stringTable.get(fileEntryArray[i].pathNameStartIndex),
                           pathName, pathNameLen) == 0) {
                //Previous filename matches with current file name, no need to create file entry
                newPmEntry->globalFileId = i;
                fileEntryArray[i].loadingId = curLoadingId;
                fileEntryArray[i].pmEntryNumbers += 1;
                hasPreviousFileNameMatch = true;
                break;
            }
        }

        return hasPreviousFileNameMatch;
    }

    void PmParser::createFileEntryUnSafe(PMEntry *newPmEntry, ssize_t loadingId, char *pathName, ssize_t pathNameLen,
                                         ssize_t scanfReadNum) {
        ssize_t newFileId = fileEntryArray.getSize();
        FileEntry *newFileEntry = fileEntryArray.pushBack(); //We should not use insertAt because globalFileId is hard-coded into dynamically generated assembly instructions.
        newPmEntry->globalFileId = newFileId;
        newFileEntry->loadingId = loadingId;
        newFileEntry->creationLoadingId = loadingId;
        newFileEntry->pmEntryNumbers += 1;
        newFileEntry->pathNameStartIndex = stringTable.getSize();
        char *rlt = stringTable.allocate(pathNameLen + 1);
        memcpy(rlt, pathName, pathNameLen + 1);//+1 because of '\0'
        newFileEntry->pathNameEndIndex = stringTable.getSize();
        newFileEntry->valid = true;//Decide later

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

    void PmParser::acruieReadLock() {
        pthread_rwlock_rdlock(&rwlock);
    }

    void PmParser::releaseReadLock() {
        pthread_rwlock_unlock(&rwlock);
    }

    const char *PmParser::getStrUnsafe(ssize_t strStart) {
        return &stringTable.get(strStart);
    }

    ssize_t PmParser::getFileEntryArraySize() {
        return fileEntryArray.getSize();
    }


}

#endif

