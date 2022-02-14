
#ifndef SCALER_MEMORYHEAP_H
#define SCALER_MEMORYHEAP_H

#include <exceptions/ScalerException.h>
#include <sys/mman.h>

namespace scaler {
    template<class OBJTYPE>
    class MemoryHeapArray {
    public:
        MemoryHeapArray(const ssize_t &initialSize = 4096) : internalArrSize(initialSize) {
            internalArr = (OBJTYPE *) mmap(NULL, initialSize * sizeof(OBJTYPE), PROT_READ | PROT_WRITE,
                                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        }

        //Cannot copy
        MemoryHeapArray &operator=(const MemoryHeapArray &) = delete;

        /**
         * Must allocate continous memory. The memory not freeed until the whole heap is deconstructed
         */
        virtual OBJTYPE *allocArr(ssize_t arraySize) {
            ssize_t newSize = size + arraySize;
            if (newSize > internalArrSize) {
                ssize_t doubleArrSize = internalArrSize * 2;
                expand(newSize > doubleArrSize ? newSize : doubleArrSize);
            }
            OBJTYPE *resultArr = internalArr + size;
            size += arraySize;
            memset(resultArr, 0, size);
            return resultArr;
        }


        ~MemoryHeapArray() {
            munmap(internalArr, internalArrSize * sizeof(OBJTYPE));
        }

    protected:
        ssize_t internalArrSize = 0;
        ssize_t size = 0;
        OBJTYPE *internalArr = nullptr;

        void expand(ssize_t newSize) {
            OBJTYPE *oldInternalArr = internalArr;
            OBJTYPE *newInternalArr = (OBJTYPE *) mmap(NULL, newSize * sizeof(OBJTYPE), PROT_READ | PROT_WRITE,
                                                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

            memcpy(newInternalArr, oldInternalArr, internalArrSize * sizeof(OBJTYPE));

            munmap(internalArr, internalArrSize * sizeof(OBJTYPE));

            internalArr = newInternalArr;
            internalArrSize = newSize;

            INFO_LOG("Memory heap array expanded.");
        }
    };
}

#endif
