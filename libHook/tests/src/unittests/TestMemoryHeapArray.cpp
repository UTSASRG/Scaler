#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/Logging.h>
#include <util/datastructure/MemoryHeapArray.h>

using namespace scaler;

template<typename T>
void testInsertion() {
    auto *memHeap = new MemoryHeapArray<T>(1);
    EXPECT_EQ(memHeap->internalArrSize, 1);
    EXPECT_EQ(memHeap->size, 0);

    T *obj1 = memHeap->allocArr(1);
    EXPECT_EQ(memHeap->size, 0);

    obj1 = memHeap->allocArr(1);
    EXPECT_EQ(memHeap->size, 2);

    obj1 = memHeap->allocArr(1);
    EXPECT_EQ(memHeap->size, 4);

    obj1 = memHeap->allocArr(10);
    EXPECT_EQ(memHeap->size, 10);
    obj1 = memHeap->allocArr(1);
    EXPECT_EQ(memHeap->size, 20);

}

struct LargeStruct {
    int64_t a1;
    int64_t a2;
    int64_t a3;
};

TEST(MemoryheapArray, insert) {
    testInsertion<char>();
    testInsertion<LargeStruct>();
}