#ifndef SCALER_ARRAY_H
#define SCALER_ARRAY_H

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <util/datastructure/ForwardIterator.h>
#include <sys/mman.h>
#include <exceptions/ScalerException.h>

namespace scaler {

    /**
     * Auto expansion array. Won't call
     * Won't call any external function for read-only operation
     * @tparam T Value type
     */
    template<typename T>
    class Array {
    public:

        //todo: Check initialized
        explicit Array(const ssize_t &initialSize = 0) : internalArrSize(initialSize), size(0) {
            if (initialSize > 0) {
                internalArr = (T *) mmap(NULL, internalArrSize * sizeof(T), PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                memset(internalArr, 0, internalArrSize * sizeof(T));
            }
        }

        Array(const Array &rho) : Array() {
            operator=(rho);
        }

        virtual ~Array() {
            if (internalArr)
                munmap(internalArr, internalArrSize * sizeof(T));
        }

        Array &operator=(const Array &rho) {
            if (&rho != this) {
                if (internalArr)
                    munmap(internalArr, internalArrSize * sizeof(T));

                internalArr = (T *) mmap(NULL, rho.internalArrSize * sizeof(T), PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                size = rho.size;
                internalArrSize = rho.internalArrSize;
                memcpy(internalArr, rho.internalArr, rho.internalArrSize * sizeof(T));
            }
            return *this;
        };

        virtual bool isEmpty() const {
            return size == 0;
        }

        virtual inline T &operator[](const ssize_t &index) {
            assert(0 <= index && index < size);
            assert(internalArr != nullptr);
            return internalArr[index];
        }

        virtual inline void erase(const ssize_t &index) {
            assert(0 <= index && index < size);
            size -= 1;
            memmove(internalArr + index, internalArr + index + 1, size - index);
        }

        virtual T *insertAt(ssize_t index) {
            assert(0 <= index && index <= size);
            if (size == internalArrSize)
                expand();
            memmove(internalArr + index + 1, internalArr + index, size - index);
            size += 1;
            return internalArr + index;
        }

        virtual inline T *pushBack() {
            return insertAt(size);
        }

        virtual inline void popBack() {
            size -= 1;
        }

        virtual inline ssize_t getSize() {
            return size;
        }

        virtual inline bool willExpand() {
            return size == internalArrSize;
        }

        virtual inline void clear() {
            size = 0;
        }

        virtual inline void setSize(ssize_t newSize) {
            size = newSize;
        }

        T *data() const {
            return internalArr;
        }

        bool release() {
            munmap(internalArr, internalArrSize * sizeof(T));
            internalArr = nullptr;
            return true;
        }

    protected:
        ssize_t internalArrSize = 0;
        ssize_t size = 0;
        T *internalArr = nullptr;

        virtual void expand() {
            T *oldInternalArr = internalArr;
            internalArr = (T *) mmap(NULL, internalArrSize * 2 * sizeof(T), PROT_READ | PROT_WRITE,
                                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            memset(internalArr, 0, internalArrSize * 2 * sizeof(T));

            memcpy(internalArr, oldInternalArr, internalArrSize * sizeof(T));
            munmap(oldInternalArr, internalArrSize * sizeof(T));
            internalArrSize *= 2;
        }
    };

    template<typename T>
    class FixedArray : public Array<T> {
    public:

        //todo: Check initialized
        explicit FixedArray(const ssize_t &initialSize = 4096) : Array<T>(initialSize) {
        }

        FixedArray(const FixedArray &rho) : FixedArray() {
            operator=(rho);
        }

    protected:
        inline void expand() override {
            fatalError("Array is expanding, try to use a larger initial size or switch to array?");
        }
    };


}
#endif
