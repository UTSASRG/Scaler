

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
}
#endif //SCALER_MATH_H
