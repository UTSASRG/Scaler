#include <util/tool/MemTool.h>
#include <elf.h>
#include <link.h>
#include <sstream>
#include <exceptions//ScalerException.h>
#include <sys/mman.h>
#include <unistd.h>
#include <zconf.h>

namespace scaler {

    MemoryTool::~MemoryTool() {

    }
}

#ifdef __linux

namespace scaler {

    //Initialize instance
    MemoryTool_Linux *MemoryTool_Linux::instance = nullptr;

    void MemoryTool_Linux::adjustMemPerm(void *startPtr, void *endPtr, int prem) {
        //Get page size
        size_t pageSize = sysconf(_SC_PAGESIZE);
        //Get Page Bound
        void *startPtrBound = GET_PAGE_BOUND(startPtr, pageSize);
        void *endPtrBound = GET_PAGE_BOUND(endPtr, pageSize);

        if (mprotect(startPtrBound,
                     (ElfW(Addr) *) endPtrBound - (ElfW(Addr) *) startPtrBound,
                     prem) != 0) {
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