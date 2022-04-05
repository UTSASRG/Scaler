#ifndef SCALER_ADDRFILEIDMAPPING_H
#define SCALER_ADDRFILEIDMAPPING_H

#include <cstdint>
#include <stdio.h>
#include <util/datastructure/Array.h>
#include "ProcInfoParser.h"

namespace scaler {

    struct Blank {
        uint64_t addrStart;
        uint64_t addrEnd;
    };
    struct PltRange {
        uint64_t addrStart;
        uint64_t addrEnd;
    };

    bool calcBestBucketSize(Array<PltRange>& pltRangeArr, ssize_t &optimalBucketSize, uint64_t &retBaseAddr);

    inline bool isBlankValid(uint64_t blankStart, uint64_t blankEnd, ssize_t &i);

}


#endif
