
#ifndef SCALER_MEMTOOL_H
#define SCALER_MEMTOOL_H

#include <type/Object.hpp>


#ifdef __linux__

//Address is aligned to page_size. Map addr to the starting of page boundary
//mprotect requires address to be a page boundary
//eg: If page size is 1024 and 1025's page boundary will be 1025/1024*1024=1024
#define GET_PAGE_BOUND(addr, page_size) (ElfW(Addr) *) ((size_t) (addr) / page_size * page_size)

//The following class is declared and defined only under linux.

#include <sys/mman.h>

namespace scaler {
    class MemoryTool_Linux : public Object {
    public:
        /**
         * Singeleton
         */
        static MemoryTool_Linux *getInst();


        void adjustMemPerm(void *startPtr, void *endPtr, int prem);

        ~MemoryTool_Linux() override;

    private:
        //Singeleton
        MemoryTool_Linux();

        static MemoryTool_Linux *instance;

    };
}
#endif

#endif //SCALER_MEMTOOL_H
