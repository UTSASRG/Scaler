
#ifndef SCALER_MATRIXWITHLOCK_H
#define SCALER_MATRIXWITHLOCK_H

#include <util/datastructure/Matrix.h>

namespace scaler {
    template<typename ValType>
    class MatrixWithLock : public Matrix<ValType> {
    public:
        MatrixWithLock(ssize_t rows, ssize_t cols) : Matrix<ValType>(rows, cols) {}

        ValType& get(const ssize_t rowId, const ssize_t colId) override {
            pthread_rwlock_rdlock(&rwlock);
            ValType &rlt = Matrix<ValType>::get(rowId, colId);
            pthread_rwlock_unlock(&rwlock);
            return rlt;
        }

        void set(const ssize_t rowId, const ssize_t colId, const ValType &val) override {
            pthread_rwlock_wrlock(&rwlock);
            Matrix<ValType>::set(rowId, colId, val);
            pthread_rwlock_unlock(&rwlock);
        }

    protected:
        pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

    };
}


#endif //SCALER_MATRIXWITHLOCK_H
