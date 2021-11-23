#ifndef SCALER_FSTACK_H
#define SCALER_FSTACK_H

namespace scaler {

    template<typename T, ssize_t initialSize>
    class FStack {
    public:
        bool isEmpty() const {
            return internalVec.size == 0;
        }

        inline void push(const T &newELem) {
            internalVec[stackTopIndex] = newELem;
            ++stackTopIndex;
        }

        inline const T &peek() {
            return internalVec[stackTopIndex-1];
        }

        inline void pop() {
            --stackTopIndex;
        }

        inline const T peekpop() {
            auto rlt = peek();
            pop();
            return rlt;
        }

        inline const ssize_t getSize() const {
            return internalVec.getSize();
        }

    protected:
        T internalVec[initialSize];
        ssize_t stackTopIndex = 0;
    };


}
#endif
