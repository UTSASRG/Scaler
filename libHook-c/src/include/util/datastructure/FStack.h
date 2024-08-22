/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#ifndef SCALER_FSTACK_H
#define SCALER_FSTACK_H

#include <cstdio>

namespace scaler {

    template<typename T, ssize_t initialSize>
    class FStack {
    public:
        inline bool isEmpty() const {
            return stackTopIndex == 0;
        }

        inline void unsafePush(const T &newELem) {
            internalVec[stackTopIndex] = newELem;
            ++stackTopIndex;
        }

        inline bool safePush(const T &newELem) {
            if (stackTopIndex < initialSize) {
                unsafePush(newELem);
                return true;
            } else {
                return false;
            }
        }

        inline const T &unsafePeek() {
            return internalVec[stackTopIndex - 1];
        }
        typedef const T* ConstT;
        inline bool safePeek(ConstT& rlt) {
            if (0 < stackTopIndex && stackTopIndex <= initialSize) {
                rlt = &unsafePeek();
                return true;
            } else {
                return false;
            }
        }

        inline void unsafePop() {
            --stackTopIndex;
        }

        inline bool safepop() {
            if (stackTopIndex > 0) {
                unsafePop();
                return true;
            } else {
                return false;
            }
        }

        inline const T unsafePeekpop() {
            auto rlt = unsafePeek();
            unsafePop();
            return rlt;
        }

        inline bool safePeekpop(ConstT& rlt) {
            bool successful=safePeek(rlt);
            if(successful){
                unsafePop();//Peek already checked condition for unsafePop
            }
            return successful;
        }

        inline const ssize_t getSize() const {
            return stackTopIndex;
        }

    protected:
        T internalVec[initialSize];
        ssize_t stackTopIndex = 0;
    };


}
#endif
