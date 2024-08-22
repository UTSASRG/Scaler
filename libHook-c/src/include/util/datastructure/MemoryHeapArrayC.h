/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#ifndef SCALER_MEMORYHEAP_H
#define SCALER_MEMORYHEAP_H

#include <exceptions/ScalerException.h>
#include <sys/mman.h>

namespace scaler {

    template<class OBJTYPE>
    class MemoryHeapArrayC {
    public:
        MemoryHeapArrayC(const ssize_t &initialSize = 4096) : internalArrSize(initialSize) {
            internalArr = (OBJTYPE *) malloc(initialSize * sizeof(OBJTYPE));
            memset(internalArr, 0, initialSize * sizeof(OBJTYPE));

        }

        //Cannot copy
        MemoryHeapArrayC &operator=(const MemoryHeapArrayC &) = delete;

        /**
         * Must allocate continous memory. The memory not freeed until the whole heap is deconstructed
         */
        virtual ssize_t allocArr(ssize_t arraySize) {
            ssize_t newSize = size + arraySize;
            if (newSize > internalArrSize) {
                ssize_t doubleArrSize = internalArrSize * 2;
                expand(newSize > doubleArrSize ? newSize : doubleArrSize);
            }
            auto tmpSize = size;
            size += arraySize;
            return tmpSize;
        }

        inline OBJTYPE &operator[](int index) const {
            assert(0 <= index && index < internalArrSize);
            return internalArr[index];
        }

        inline MemoryHeapElem<OBJTYPE> getPtr(ssize_t index) {
            return MemoryHeapElem<OBJTYPE>(*this, index);
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
            //memset(newInternalArr + internalArrSize, 0, (newSize - internalArrSize) * sizeof(OBJTYPE));

            munmap(internalArr, internalArrSize * sizeof(OBJTYPE));

            internalArr = newInternalArr;
            internalArrSize = newSize;

            INFO_LOG("Memory heap array expanded.");
        }
    };

    template<class OBJTYPE>
    class MemoryHeapElem {
    public:
        MemoryHeapElem(MemoryHeapArray<OBJTYPE> &memoryHeapArray, ssize_t &index) : memoryHeapArray(&memoryHeapArray),
                                                                                    index(index) {

        }

        inline OBJTYPE &operator*() {
            return memoryHeapArray->operator[](index);
        }
        inline OBJTYPE* operator->() {
            return &memoryHeapArray->operator[](index);
        }

        inline ssize_t getIndex() {
            return index;
        }


    protected:
        MemoryHeapArray<OBJTYPE> *memoryHeapArray;
        ssize_t index;
    };

}

#endif
