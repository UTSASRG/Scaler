#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/Logging.h>
#include <util/datastructure/MemoryHeapList.h>

using namespace scaler;


template<typename T>
void testInsertion() {
    auto *memHeap = new MemoryHeapList<T>(1);

    EXPECT_EQ(memHeap->chunkHead->nextChunk, nullptr);
    EXPECT_EQ(memHeap->chunkHead->size, 0);
    EXPECT_EQ(memHeap->chunkHead->internalArrSize, 1);

    T *obj1 = memHeap->malloc();

    EXPECT_EQ(memHeap->chunkHead->nextChunk, nullptr);
    EXPECT_EQ(memHeap->chunkHead->size, 1);
    EXPECT_EQ(memHeap->chunkHead->internalArrSize, 1);

    T *obj2 = memHeap->malloc();

    EXPECT_NE(memHeap->chunkHead->nextChunk, nullptr);
    EXPECT_EQ(memHeap->chunkHead->size, 1);
    EXPECT_EQ(memHeap->chunkHead->internalArrSize, 2);

    T *obj3 = memHeap->malloc();

    T* obj1Cpy=obj1;
    memHeap->free(obj1);
    T* obj2Cpy=obj2;
    memHeap->free(obj2);
    T* obj3Cpy=obj3;
    memHeap->free(obj3);


    //Currently it will reset the value of obj1-3 to avoid mistake. The following tests will not be used.

    EXPECT_EQ(obj3Cpy, memHeap->malloc());
    EXPECT_EQ(obj2Cpy, memHeap->malloc());
    EXPECT_EQ(obj1Cpy, memHeap->malloc());
}

struct LargeStruct {
    int64_t a1;
    int64_t a2;
    int64_t a3;
};

TEST(MemoryheapList, insert) {
    testInsertion<char>();
    testInsertion<LargeStruct>();
}


