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
        size_t memoryLength = (ceil(((uint8_t *) endPtrBound - (uint8_t *) startPtrBound) / (double)pageSize)) * pageSize;
        //printf("mprotect %p-%p\n", startPtrBound, (uint8_t *) startPtrBound + memoryLength);
        if (mprotect(startPtrBound, memoryLength, prem) != 0) {
            std::stringstream ss;
            ss << "Could not change the process memory permission at " << startPtrBound << " - " << endPtrBound;
            throwScalerException(ss.str().c_str());
        }

    }

    MemoryTool_Linux *scaler::MemoryTool_Linux::getInst() {
        if (MemoryTool_Linux::instance == nullptr) {
            MemoryTool_Linux::instance = new MemoryTool_Linux();
        }
        return MemoryTool_Linux::instance;
    }

    MemoryTool_Linux::MemoryTool_Linux() {

    }

    MemoryTool_Linux::~MemoryTool_Linux() = default;


}
#endif