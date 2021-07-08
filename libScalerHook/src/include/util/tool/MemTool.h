
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
#include "ProcInfoParser.h"

namespace scaler {
    class MemoryTool: public Object {
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
        virtual void *binCodeSearch(void *target, size_t targetSize, void *keyword, size_t keywordSize);

        /**
         * Search the starting and ending address of a loaded section in ELF image
         * @param segPtrInFile: The pointer to the starting address of a memory segment
         * @param firstEntrySize: # of bytes to search. The size of the first entry would be ideal.
         * because memory alignment typically happens after one entry
         * @param segments Specify wich segment to search
         */
        virtual void *searchBinInMemory(void *segPtrInFile, size_t firstEntrySize, const std::vector<PMEntry_Linux> &segments);

    protected:
        //Singeleton
        MemoryTool();

        static MemoryTool *instance;

    };


}
#endif

#endif //SCALER_MEMTOOL_H
