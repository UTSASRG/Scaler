
#ifndef SCALER_MEMORYHEAP_H
#define SCALER_MEMORYHEAP_H

#include <exceptions/ScalerException.h>

namespace scaler {
    template<class OBJTYPE>
    class MemoryHeap {

        virtual OBJTYPE *malloc() {
            fatalError("malloc not implemented");
        }

        virtual void free() {
            fatalError("free not implemented");

        }

        /**
         * Must allocate continous memory
         * @return array
         */
        virtual OBJTYPE *allocArr(ssize_t arraySize) {
            fatalError("allocArr not implemented");

        }

        virtual void freeArr(ssize_t objSize) {
            fatalError("freeArr not implemented");

        }


    };
}

#endif
