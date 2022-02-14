#ifndef SCALER_VECTOR_H
#define SCALER_VECTOR_H

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <util/datastructure/ForwardIterator.h>

namespace scaler {
    template<typename T>
    class Vector;

    template<typename T>
    class VectorIterator : public ForwardIterator<T, VectorIterator<T>>, ReverseIterator<T, VectorIterator<T>> {
    public:
        using Vector_ = Vector<T>;

        explicit VectorIterator(Vector_ *vector) : vector(vector), index(0) {
        }

        VectorIterator &operator++() override {
            //End iterator points to index==size()
            assert(index <= vector->getSize());
            ++index;
            return *this;
        }

        const VectorIterator operator++(int) override {
            return operator++();
        }

        VectorIterator &operator--() override {
            //rend iterator points index==-1
            assert(index >= -1);
            --index;
            return *this;
        }

        const VectorIterator operator--(int i) override {
            return operator--();
        }

        bool operator!=(const VectorIterator<T> &rho) const override {
            return !operator==(rho);
        }

        bool operator==(const VectorIterator<T> &rho) const override {
            return vector == rho.vector && index == rho.index;
        }

        T &operator*() override {
            assert(0 <= index && index <= vector->getSize());
            return vector->operator[](index);
        }

    protected:
        friend class Vector<T>;

        // which bucket at the moment? [0, nbucket-1]
        Vector_ *vector;
        ssize_t index;
    };

    /**
     * Auto expansion array, similar to STL
     * Won't call any external function for read-only operation
     * @tparam T Value type
     */
    template<typename T>
    class Vector : public Iteratable<VectorIterator<T>>, ReverseIteratable<VectorIterator<T>> {
    public:

        //todo: Check initialized
        explicit Vector(const ssize_t &initialSize = 4096) : internalArrSize(initialSize), size(0), beginIter(this),
                                                             endIter(this), rbeginIter(this), rendIter(this) {
            if (initialSize > 0) {
                internalArr = new T[initialSize];
            }
        }

        Vector(const Vector &rho) : Vector() {
            operator=(rho);
        }

        virtual ~Vector() {
            if (internalArr)
                delete[] internalArr;
        }

        Vector &operator=(const Vector &rho) {
            if (&rho != this) {
                delete[] internalArr;
                internalArr = nullptr;
                internalArr = new T[rho.internalArrSize];
                size = rho.size;
                internalArrSize = rho.internalArrSize;
                for (int i = 0; i < rho.size; ++i) {
                    internalArr[i] = rho.internalArr[i];
                }
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
            for (int i = index + 1; i < size; ++i) {
                internalArr[i - 1] = internalArr[i];
            }
            size -= 1;
        }

        virtual void insertAt(const ssize_t &index, const T &newElem) {
            assert(0 <= index && index <= size);
            if (size == internalArrSize)
                expand();
            for (int i = size - 1; i >= index; --i) {
                internalArr[i + 1] = internalArr[i];
            }
            internalArr[index] = newElem;
            size += 1;
        }

        virtual inline void pushBack(const T &newElem) {
            insertAt(size, newElem);
        }

        virtual inline void pushBack(const T &&newElem) {
            insertAt(size, std::move(newElem));
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

        const VectorIterator<T> &begin() override {
            beginIter.index = size == 0 ? -1 : 0;
            return beginIter;
        }

        const VectorIterator<T> &end() override {
            endIter.index = size == 0 ? -1 : size;
            return endIter;
        }

        const VectorIterator<T> &rbegin() override {
            rbeginIter.index = size == 0 ? -1 : size - 1;
            return rbeginIter;
        }

        const VectorIterator<T> &rend() override {
            rendIter.index = -1;
            return rendIter;
        }

        const T *data() const {
            return internalArr;
        }

    protected:
        ssize_t internalArrSize = 0;
        ssize_t size = 0;
        T *internalArr = nullptr;

        VectorIterator<T> beginIter;
        VectorIterator<T> endIter;
        VectorIterator<T> rbeginIter;
        VectorIterator<T> rendIter;
        bool saveContentOnExit;
        virtual void expand() {
            T *oldInternalArr = internalArr;
            T *newInternalArr = new T[internalArrSize * 2];
            for (int i = 0; i < internalArrSize; ++i) {
                newInternalArr[i] = oldInternalArr[i];
            }
            internalArrSize *= 2;
            delete[] oldInternalArr;
            oldInternalArr = nullptr;
            internalArr = newInternalArr;
        }
    };


}
#endif
