
#ifndef SCALER_MEMTOOL_H
#define SCALER_MEMTOOL_H

#include <type/Object.hpp>


#ifdef __linux__

//Address is aligned to page_size. Map addr to the starting of page boundary
//mprotect requires address to be a page boundary
//eg: If page size is 1024 and 1025's page boundary will be 1025/1024*1024=1024
#define GET_PAGE_BOUND(addr, page_size) (ElfW(Addr) *) ((ssize_t) (addr) / page_size * page_size)

//The following class is declared and defined only under linux.

#include <sys/mman.h>
#include "ProcInfoParser.h"
#include "Logging.h"

namespace scaler {
    class MemoryTool : public Object {
    public:
        /**
         * Singeleton
         */
        static MemoryTool *getInst();


        ~MemoryTool() override;

        /**
         * Search for a binary keyword in another array
         * @param target Target array to search
         * @param keyword Keyword to search
         * @return The pointer of the first starting address of keyword in target
         */
        virtual void *binCodeSearch(void *target, ssize_t targetSize, void *keyword, ssize_t keywordSize);

        /**
         * Search the starting and ending address of a loaded section in ELF image
         * @param segPtrInFile: The pointer to the starting address of a memory segment
         * @param firstEntrySize: # of bytes to search. The size of the first entry would be ideal.
         * because memory alignment typically happens after one entry
         * @param segments Specify wich segment to search
         */
        virtual void *
        searchBinInMemory(void *segPtrInFile, ssize_t firstEntrySize, const std::vector<PMEntry_Linux> &segments,
                          void *boundStartAddr, void *boundEndAddr);

        void printStackInfo() {


            size_t stacksize = -1;
            void *stack_addr = nullptr;
            pthread_attr_t attr;
            pthread_getattr_np(pthread_self(), &attr);

            // 获取缺省的堆栈地址和大小
            pthread_attr_getstack(&attr, &stack_addr, &stacksize);
            std::cout << "default stack addr: " << stack_addr << std::endl;
            std::cout << "default stack size: " << stacksize << std::endl;

            size_t used, avail;

            size_t stack_size;
            size_t rsp_val;
            asm("movq %%rsp, %0" : "=m"(rsp_val) :);
            pthread_attr_getstack(&attr, &stack_addr, &stack_size);
            pthread_attr_destroy(&attr);
            avail = rsp_val - (size_t) stack_addr;
            used = stack_size - avail;
            DBG_LOGS(
                    "Thread %lu stack info: rspVal=%zu stacktop=%p stackbottom=%p  used = %zu, avail = %zuMB, total = %zuMB ",
                    pthread_self(),rsp_val, (uint8_t*)stack_addr-(size_t)stack_size, (uint8_t*)stack_addr, used, avail/1024/1024,
                    stack_size/1024/1024);
        }

    protected:
        //Singeleton
        MemoryTool();

        static MemoryTool *instance;

    };


}
#endif

#endif //SCALER_MEMTOOL_H
