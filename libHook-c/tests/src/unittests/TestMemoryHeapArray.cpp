/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/Logging.h>
#include <util/datastructure/MemoryHeapArray.h>

using namespace scaler;

template<typename T>
void testInsertion() {
//    auto *memHeap = new MemoryHeapArray<T>(1);
//    EXPECT_EQ(memHeap->internalArrSize, 1);
//    EXPECT_EQ(memHeap->size, 0);
//
//    T *obj1 = memHeap->allocArr(1);
//    EXPECT_EQ(memHeap->size, 0);
//
//    obj1 = memHeap->allocArr(1);
//    EXPECT_EQ(memHeap->size, 2);
//
//    obj1 = memHeap->allocArr(1);
//    EXPECT_EQ(memHeap->size, 4);
//
//    obj1 = memHeap->allocArr(10);
//    EXPECT_EQ(memHeap->size, 10);
//    obj1 = memHeap->allocArr(1);
//    EXPECT_EQ(memHeap->size, 20);

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