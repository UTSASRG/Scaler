#ifndef SCALER_STACK_H
#define SCALER_STACK_H

#include <cassert>
#include "Vector.h"

namespace scaler {

    /**
     * Auto expansion array, similar to STL
     * Won't call any external function for read-only operation
     * @tparam T Value type
     */
    template<typename T>
    class Stack {
    public:
        //todo: Check initialized
        Stack(const ssize_t &initialSize = 2) : internalVec(initialSize) {

        }

        Stack(const Stack &rho) = delete;

        Stack &operator=(const Stack &rho) = delete;

        bool isEmpty() {
            return internalVec.size == 0;
        }

        inline void push(const T &newELem) {
            internalVec.insertAt(internalVec.getSize(), newELem);
        }

        inline const T &peek() {
            return internalVec[internalVec.getSize() - 1];
        }

        inline void pop() {
            internalVec.remove(internalVec.getSize() - 1);
        }

        inline const T peekpop() {
            auto rlt = peek();
            pop();
            return rlt;
        }

    protected:
        Vector<T> internalVec;
    };


}
#endif
