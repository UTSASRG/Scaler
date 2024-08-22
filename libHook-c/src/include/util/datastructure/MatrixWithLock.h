
/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
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
