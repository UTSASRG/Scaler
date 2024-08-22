/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/

#ifndef SCALER_MATH_H
#define SCALER_MATH_H

#include <ctype.h>

namespace scaler {
    inline void calcMean(const int64_t &x, int64_t &counter, double &mean) {
        counter++;
        double delta = x - mean;
        mean += delta / counter;
        //var += (double) (n - 1) / n * delta * delta;
    }

    template<typename T>
    inline const T &max(const T &a, const T &b) {
        return a > b ? a : b;
    }

    template<typename T>
    inline const T &min(const T &a, const T &b) {
        return a > b ? b : a;
    }
}
#endif //SCALER_MATH_H
