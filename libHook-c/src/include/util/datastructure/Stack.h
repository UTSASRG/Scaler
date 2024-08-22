/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
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
