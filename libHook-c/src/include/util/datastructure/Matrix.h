/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#ifndef SCALER_MATRIX_H
#define SCALER_MATRIX_H
namespace scaler {

    template<typename ValType>
    class Matrix {
    public:

        virtual inline ssize_t getRows() {
            return rows;
        }

        virtual inline ssize_t getCols() {
            return cols;
        }

        virtual inline ValType& get(const ssize_t rowId, const ssize_t colId) {
            return data[rowId][colId];
        }

        virtual inline void set(const ssize_t rowId, const ssize_t colId, const ValType &val) {
            data[rowId][colId] = val;
        }

        virtual ~Matrix() {
            free(data);
            data = nullptr;
        }

        Matrix(ssize_t rows, ssize_t cols) : rows(rows), cols(cols) {
            data = static_cast<int64_t **>( malloc(
                    (rows + rows * cols) * sizeof(int64_t)));
            if (!data) {
                fatalError("Failed to allocate memory for Matrix");
                exit(-1);
            }
            for (int i = 0; i < rows; ++i) {
                //todo: slow speed, change to memset?
                data[i] = reinterpret_cast<int64_t *>(data + rows + i * cols);
            }
            memset(data + rows, 0, rows * cols * sizeof(int64_t));
        }

    protected:
        ValType **data = nullptr;
        ssize_t rows = -1;
        ssize_t cols = -1;
    };
}

#endif //SCALER_MATRIX_H
