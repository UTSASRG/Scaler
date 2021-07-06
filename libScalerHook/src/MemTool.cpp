#include <util/tool/MemTool.h>


#ifdef __linux

#include <elf.h>
#include <link.h>
#include <cmath>
#include <sstream>
#include <exceptions//ScalerException.h>
#include <sys/mman.h>
#include <unistd.h>

namespace scaler {

    //Initialize instance
    MemoryTool_Linux *MemoryTool_Linux::instance = nullptr;

    void MemoryTool_Linux::adjustMemPerm(void *startPtr, void *endPtr, int prem) {
        //Get page size
        size_t pageSize = sysconf(_SC_PAGESIZE);
        //Get Page Bound
        void *startPtrBound = GET_PAGE_BOUND(startPtr, pageSize);
        void *endPtrBound = GET_PAGE_BOUND(endPtr, pageSize);
        if (startPtrBound == endPtrBound)
            endPtrBound = (uint8_t *) startPtrBound + pageSize;

        //todo:(uint8_t *) endPtrBound - (uint8_t  *) startPtrBound,
        size_t memoryLength =
                (ceil(((uint8_t *) endPtrBound - (uint8_t *) startPtrBound) / (double) pageSize)) * pageSize;
        //printf("mprotect %p-%p\n", startPtrBound, (uint8_t *) startPtrBound + memoryLength);
        if (mprotect(startPtrBound, memoryLength, prem) != 0) {
            std::stringstream ss;
            ss << "Could not change the process memory permission at " << startPtrBound << " - " << endPtrBound;
            throwScalerException(ss.str().c_str());
        }

    }

    MemoryTool_Linux *scaler::MemoryTool_Linux::getInst() {
        //MemoryTool_Linux::instance memory leak
        if (MemoryTool_Linux::instance == nullptr) {
            MemoryTool_Linux::instance = new MemoryTool_Linux();
        }
        return MemoryTool_Linux::instance;
    }

    MemoryTool_Linux::MemoryTool_Linux() {

    }

    void *MemoryTool::searchBinInMemory(void *segPtrInFile, size_t firstEntrySize,
                                        const std::vector<PMEntry_Linux> &segments) {
        void *rltAddr = nullptr;

        for (int i = 0; i < segments.size(); ++i) {
            rltAddr = binCodeSearch(segments[i].addrStart, segments[i].length, segPtrInFile, firstEntrySize);
            if (rltAddr)
                break;
        }
        return rltAddr;
    }

    void *MemoryTool::binCodeSearch(void *target, size_t targetSize, void *keyword, size_t keywordSize) {
        //Convert it to uint8* so that we can perform arithmetic operation on those pointers
        uint8_t *kwd = static_cast<uint8_t *>(keyword);
        uint8_t *tgt = static_cast<uint8_t *>(target);

        int i = 0, j = 0; //i is the index in target and j is the index in keyword
        uint8_t *beg = nullptr; //Markes the begging of the match

        while (i < targetSize && j < keywordSize) {
            if (tgt[i] == kwd[j]) {
                if (beg == nullptr) {
                    //First match. It's a potential starting position.
                    beg = tgt + i;
                }
                ++j;
            } else {
                //If tgt[i] != kwd[j] it means this is not the correct keyword. Reset beg and j.
                beg = nullptr;
                j = 0;
            }
            ++i;
        }
        // If j==keywordSize it means the previous loop exit because of this. Then it means a match if found.
        return j == keywordSize ? beg : nullptr;
    }

    MemoryTool_Linux::~MemoryTool_Linux() = default;


    MemoryTool *MemoryTool::getInst() {
        if (MemoryTool::instance == nullptr) {
            MemoryTool::instance = new MemoryTool();
        }
        return MemoryTool::instance;
    }

    //Initialize instance
    MemoryTool *MemoryTool::instance = nullptr;

    MemoryTool::~MemoryTool()= default;

    MemoryTool::MemoryTool() {

    }
}
#endif