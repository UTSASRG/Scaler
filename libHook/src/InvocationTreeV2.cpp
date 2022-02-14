#include <type/InvocationTreeV2.h>

scaler::InvocationTreeV2::InvocationTreeV2(scaler::SymID &hookedId, uint64_t &duration) : hookedId(hookedId),
                                                                                          duration(duration),
                                                                                          children(nullptr),
                                                                                          childrenSize(0) {
}

void scaler::InvocationTreeV2::setChildren(scaler::InvocationTreeV2 *childrenMem, ssize_t childrenArraySize) {
    children = childrenMem;
    childrenSize = childrenArraySize;
}

scaler::InvocationTreeV2::~InvocationTreeV2() {

}
