#include <util/tool/MemTool.h>
#include <elf.h>
#include <link.h>
#include <sstream>
#include <exceptions//ScalerException.h>
#include <sys/mman.h>
#include <bits/confname.h>
#include <zconf.h>

#ifdef __linux

namespace scaler {
    //Initialize instance
    MemoryTool_Linux *MemoryTool_Linux::instance = nullptr;

    void MemoryTool_Linux::adjustMemPerm(void *startPtr, void *endPtr, int prem) {
        size_t pageSize = sysconf(_SC_PAGESIZE);
        void *alignedStartPtr = ALIGN_ADDR(startPtr, pageSize);
        void *alignedEndPtr = ALIGN_ADDR(endPtr, pageSize);

        if (mprotect(alignedStartPtr, (ElfW(Addr) *) alignedEndPtr - (ElfW(Addr) *)
                alignedStartPtr, prem) != 0) {
            std::stringstream ss;
            ss << "Could not change the process memory permission at " << alignedStartPtr << " - " << alignedEndPtr;
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
}
#endif