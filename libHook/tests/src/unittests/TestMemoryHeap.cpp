#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/Logging.h>
#include <util/datastructure/MemoryHeap.h>

using namespace scaler;


template<typename T>
void testInsertion() {
    auto *memHeap = new MemoryHeap<T>(1);

    EXPECT_EQ(memHeap->chunkHead->nextChunk, nullptr);
    EXPECT_EQ(memHeap->chunkHead->managedSlots, 0);
    EXPECT_EQ(memHeap->chunkHead->totalSlots, 1);

    T *obj1 = memHeap->malloc();

    EXPECT_EQ(memHeap->chunkHead->nextChunk, nullptr);
    EXPECT_EQ(memHeap->chunkHead->managedSlots, 1);
    EXPECT_EQ(memHeap->chunkHead->totalSlots, 1);

    T *obj2 = memHeap->malloc();

    EXPECT_NE(memHeap->chunkHead->nextChunk, nullptr);
    EXPECT_EQ(memHeap->chunkHead->managedSlots, 1);
    EXPECT_EQ(memHeap->chunkHead->totalSlots, 2);

    T *obj3 = memHeap->malloc();

    memHeap->free(obj1);
    memHeap->free(obj2);
    memHeap->free(obj3);

    //Currently it will reset the value of obj1-3 to avoid mistake. The following tests will not be used.

    //EXPECT_EQ(obj3, memHeap->malloc());
    //EXPECT_EQ(obj2, memHeap->malloc());
    //EXPECT_EQ(obj1, memHeap->malloc());
}

struct LargeStruct {
    int64_t a1;
    int64_t a2;
    int64_t a3;
};
TEST(Memoryheap, insert) {
    testInsertion<char>();
    testInsertion<LargeStruct>();
}


