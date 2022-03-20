#ifndef SCALER_INVOCATIONTREE_H
#define SCALER_INVOCATIONTREE_H

#include <cstdint>
#include <util/datastructure/Vector.h>
#include <util/datastructure/MemoryHeapArray.h>
#include "ExtSymInfo.h"

namespace scaler {
    //Check issue 35 https://github.com/UTSASRG/Scaler/issues/35
    struct InvocationTree {
        SymID scalerId; //4bytes
        uint64_t duration; //8byte
        InvocationTree *children; //8bytes
        ssize_t childrenSize;
    };

}

#endif //SCALER_INVOCATIONTREE_H
