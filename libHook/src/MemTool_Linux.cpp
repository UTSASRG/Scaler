#include <util/tool/MemoryTool_Linux.h>


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

    bool MemoryTool_Linux::adjustMemPerm(void *startPtr, void *endPtr, int prem) {
        //Get page size
        ssize_t pageSize = sysconf(_SC_PAGESIZE);
        //Get Page Bound
        void *startPtrBound = GET_PAGE_BOUND(startPtr, pageSize);
        void *endPtrBound = GET_PAGE_BOUND(endPtr, pageSize);
        if (startPtrBound == endPtrBound)
            endPtrBound = (uint8_t *) startPtrBound + pageSize;

        //todo:(uint8_t *) endPtrBound - (uint8_t  *) startPtrBound,
        ssize_t memoryLength =
                (ceil(((uint8_t *) endPtrBound - (uint8_t *) startPtrBound) / (double) pageSize)) * pageSize;
        if (mprotect(startPtrBound, memoryLength, prem)!=0) {
            ERR_LOGS("Could not change the process memory permission at %p-%p because: %s", startPtrBound, endPtrBound,strerror(errno));
            return false;
        }
        return true;
    }

    MemoryTool_Linux *scaler::MemoryTool_Linux::getInst() {
        //MemoryTool_Linux::instance memory leak
        if (MemoryTool_Linux::instance == nullptr) {
            MemoryTool_Linux::instance = new MemoryTool_Linux();
            if (!MemoryTool_Linux::instance) {
                fatalError("Cannot allocate memory for MemoryTool_Linux");
            }
        }
        return MemoryTool_Linux::instance;
    }

    MemoryTool_Linux::MemoryTool_Linux() {

    }

    MemoryTool_Linux::~MemoryTool_Linux() = default;

}
#endif