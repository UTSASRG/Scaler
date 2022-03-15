#ifndef SCALER_INVOCATIONTREEV2_H
#define SCALER_INVOCATIONTREEV2_H

#include <cstdint>
#include <util/datastructure/Vector.h>
#include <util/hook/hook.hh>
#include <util/datastructure/MemoryHeapArray.h>

namespace scaler {
    //Check issue 35 https://github.com/UTSASRG/Scaler/issues/35
    struct InvocationTreeV2 {
        SymID scalerId; //4bytes
        uint64_t duration; //8byte
        InvocationTreeV2 *children; //8bytes
        ssize_t childrenSize;
    };

}

#endif //SCALER_INVOCATIONTREEV2_H
