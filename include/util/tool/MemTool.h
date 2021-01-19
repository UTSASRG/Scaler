
#ifndef SCALER_MEMTOOL_H
#define SCALER_MEMTOOL_H

#include <type/Object.hpp>

#define ALIGN_ADDR(addr, page_size) (ElfW(Addr) *) ((size_t) (addr) / page_size * page_size)

namespace scaler {
    /**
     *
     */
    class MemoryTool : public Object {
    public:
        /**
        * Change the permission of a memory location.
        */
        virtual void adjustMemPerm(void *startPtr, void *endPtr, int prem) = 0;

        ~MemoryTool() override = 0;
    };
}

#ifdef __linux__

#include <sys/mman.h>

namespace scaler {
    class MemoryTool_Linux : public MemoryTool {
    public:
        /**
         * Singeleton
         */
        static MemoryTool_Linux *getInst();

        MemoryTool_Linux(MemoryTool_Linux &) = delete;

        MemoryTool_Linux(MemoryTool_Linux &&) = delete;

        //todo: Write unit test
        /**
         * Change the permission of a memory location.
         * This function automatically aligns memory as required.
         *
         * @param startPtr Starting pointer of a memory location
         * @param endPtr End pointer of a memory location
         * @param prem Permission flag for mmprotect
         */
        void adjustMemPerm(void *startPtr, void *endPtr, int prem) override;

        ~MemoryTool_Linux() override;

    private:
        MemoryTool_Linux();

        static MemoryTool_Linux *instance;

    };
}
#endif

#endif //SCALER_MEMTOOL_H
