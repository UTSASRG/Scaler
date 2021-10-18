#ifndef SCALER_VECTOR_H
#define SCALER_VECTOR_H

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace scaler {

    /**
     * Auto expansion array, similar to STL
     * Won't call any external function for read-only operation
     * @tparam T Value type
     */
    template<typename T>
    class Vector {
    public:
        //todo: Check initialized
        Vector(const ssize_t &initialSize = 2) : internalArrSize(initialSize), size(0) {
            internalArr = (T *) calloc(sizeof(T), initialSize);
        }

        Vector(const Vector &rho) = delete;

        Vector &operator=(const Vector &rho) = delete;

        bool isEmpty() {
            return size == 0;
        }

        inline const T &operator[](const ssize_t &index) {
            return internalArr[index];
        }

        inline void remove(const ssize_t &index) {
            assert(0 <= index && index < size);
            for (int i = index + 1; i < size; ++i) {
                internalArr[i - 1] = internalArr[i];
            }
            size -= 1;
        }

        void expand() {
            T *oldInternalArr = internalArr;
            T *newInternalArr = (T *) calloc(sizeof(T), internalArrSize * 2);
            memcpy((void *) newInternalArr, oldInternalArr, internalArrSize * sizeof(T));
            internalArrSize *= 2;
            free(oldInternalArr);
            oldInternalArr = nullptr;
            internalArr = newInternalArr;
        }

        inline void insertAt(const ssize_t &index, const T &newElem) {
            assert(0 <= index && index <= size);
            if (size == internalArrSize)
                expand();
            for (int i = size - 1; i >= index; --i) {
                internalArr[i + 1] = internalArr[i];
            }
            internalArr[index] = newElem;
            size += 1;
        }

        inline ssize_t getSize() {
            return size;
        }


    protected:
        ssize_t internalArrSize;
        ssize_t size;
        T *internalArr;
    };


}
#endif
