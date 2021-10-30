#ifndef SCALER_STACK_H
#define SCALER_STACK_H

#include <cassert>
#include "Vector.h"

namespace scaler {

    template<typename T>
    class Stack {
    public:
        Stack(ssize_t initialSize = 0) : internalVec(initialSize) {
        }

        bool isEmpty() const {
            return internalVec.size == 0;
        }

        inline void push(const T &newELem) {
            internalVec.insertAt(internalVec.getSize(), newELem);
        }

        inline const T &peek() {
            //assert(internalVec.getSize() > 0);
            return internalVec[internalVec.getSize() - 1];
        }

        inline void pop() {
            //assert(internalVec.getSize() > 0);
            internalVec.erase(internalVec.getSize() - 1);
        }

        inline const T peekpop() {
            auto rlt = peek();
            pop();
            return rlt;
        }

        inline const ssize_t getSize() const{
            return internalVec.getSize();
        }

    protected:
        Vector <T> internalVec;
    };


}
#endif
