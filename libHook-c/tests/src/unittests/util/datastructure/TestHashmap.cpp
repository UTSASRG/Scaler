/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <gtest/gtest.h>
#include <util/datastructure/Hashmap.hh>

TEST(HashMap, InsertQuery) {
    //Basic stroe and query

    scaler::HashMap<int, int> hashMap(20);

    for (int i = 0; i < 20; ++i) {
        hashMap.put(i, i);
    }
    //Each bucket should have only one value
    for (int i = 0; i < 20; ++i) {
        ASSERT_EQ(hashMap.buckets[i].getSize(), 1);
    }

    //Put existing element, but refuse to replace
    ASSERT_DEATH(hashMap.put(0, 0, false), ".*Assertion `replace' failed.*");

    for (int i = 0; i < 5; ++i) {
        hashMap.put(i, i);
    }

    //First 5 buckets should one value, since the key already exist
    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(hashMap.buckets[i].getSize(), 1);
    }

    for (int i = 20; i < 25; ++i) {
        hashMap.put(i, i);
    }

    //First 5 buckets should two values now
    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(hashMap.buckets[i].getSize(), 2);
    }
    for (int i = 6; i < 20; ++i) {
        ASSERT_EQ(hashMap.buckets[i].getSize(), 1);
    }

    for (int i = 0; i < 25; ++i) {
        ASSERT_EQ(hashMap.get(i), i);
        ASSERT_EQ(*hashMap.getPtr(i), i);
    }

    //Get non-existing
    ASSERT_DEATH(hashMap.get(25), ".*rltPtr != nullptr.*");

    scaler::HashMap<int, int> hashMap1(1);

    for (int i = 0; i < 20; ++i) {
        hashMap1.put(i, i);
    }
    ASSERT_EQ(hashMap1.buckets[0].getSize(), 20);

    for (int i = 0; i < 20; ++i) {
        ASSERT_EQ(hashMap1.get(i), i);
        ASSERT_EQ(*hashMap1.getPtr(i), i);
    }


}


TEST(HashMap, Erase) {
    //Basic stroe and query

    scaler::HashMap<int, int> hashMap(20);

    for (int i = 0; i < 25; ++i) {
        hashMap.put(i, i);
    }


    //Erase a value from one-element buckets
    hashMap.erase(15);
    ASSERT_DEATH(hashMap.get(15), ".*rltPtr != nullptr.*");

    //Erase a value from two-element buckets
    hashMap.erase(2);
    ASSERT_DEATH(hashMap.get(2), ".*rltPtr != nullptr.*");

}

TEST(HashMap, Iteration) {
    scaler::HashMap<int, int> hashMap20(20);
    scaler::HashMap<int, int> hashMap1(1);

    auto hm20Beg = hashMap20.begin();
    auto hm20End = hashMap20.end();
    ASSERT_TRUE(hm20Beg == hm20Beg);
    ASSERT_TRUE(hm20End == hm20End);
    ASSERT_TRUE(hm20Beg == hm20End);
    ASSERT_DEATH(*hm20Beg, ".*curBucketEntry != curBucket->getTail().*");
    ASSERT_DEATH(*hm20End, ".*curBucketEntry != curBucket->getTail().*");

    auto hm1Beg = hashMap1.begin();
    auto hm1End = hashMap1.end();
    ASSERT_TRUE(hm1Beg == hm1Beg);
    ASSERT_TRUE(hm1End == hm1End);
    ASSERT_TRUE(hm1Beg == hm1End);
    ASSERT_DEATH(*hm1Beg, ".*curBucketEntry != curBucket->getTail().*");
    ASSERT_DEATH(*hm1End, ".*curBucketEntry != curBucket->getTail().*");

    hashMap20.put(12, 12);
    hashMap1.put(12, 12);

    hm20Beg = hashMap20.begin();
    hm20End = hashMap20.end();
    ASSERT_TRUE(hm20Beg == hm20Beg);
    ASSERT_TRUE(hm20End == hm20End);
    ASSERT_TRUE(hm20Beg != hm20End);
    ASSERT_EQ(*hm20Beg, 12);
    ASSERT_TRUE(++hm20Beg == hm20End);


    hm1Beg = hashMap1.begin();
    hm1End = hashMap1.end();
    ASSERT_TRUE(hm1Beg == hm1Beg);
    ASSERT_TRUE(hm1End == hm1End);
    ASSERT_TRUE(hm1Beg != hm1End);
    ASSERT_EQ(*hm1Beg, 12);
    ASSERT_TRUE(hm1Beg++ == hm1End);


    hashMap1.erase(12);
    hashMap20.erase(12);

    for (int i = 12; i < 20; i += 2) {
        hashMap1.put(i, i);
        hashMap20.put(i, i);
    }

    char rlt[] = {18, 16, 14, 12};
    int index = 0;
    for (auto elem = hashMap1.begin(); elem != hashMap1.end(); ++elem) {
        ASSERT_EQ(*elem, rlt[index++]);
    }
    index = 0;
    for (auto &elem:hashMap1) {
        ASSERT_EQ(elem, rlt[index++]);
    }

    char rlt1[] = {12, 14, 16, 18};

    index = 0;
    for (auto elem = hashMap20.begin(); elem != hashMap20.end(); ++elem) {
        ASSERT_EQ(*elem, rlt1[index++]);
    }

    index = 0;
    for (auto& elem:hashMap20) {
        ASSERT_EQ(elem, rlt1[index++]);
    }

    //Iterator copy
    auto iter20A = hashMap20.begin();
    ASSERT_EQ(*iter20A, 12);
    ++iter20A;
    ASSERT_EQ(*iter20A, 14);
    auto iter20B = iter20A;
    ++iter20A;
    ASSERT_EQ(*iter20A, 16);
    ++iter20A;
    ASSERT_EQ(*iter20A, 18);
    ++iter20B;
    ASSERT_EQ(*iter20B, 16);
    ++iter20B;

}


TEST(Hashmap, copyconstruct) {
    scaler::HashMap<int, int> hmA(15);

    for (int i = 0; i < 20; ++i) {
        hmA.put(i, i);
    }

    scaler::HashMap<int, int> hmB = hmA;
    scaler::HashMap<int, int> hmC(hmA);

    for (int i = 0; i < 20; ++i) {
        ASSERT_EQ(hmA.get(i), i);
        hmA.erase(i);
    }

    for (int i = 0; i < 20; ++i) {
        ASSERT_EQ(hmB.get(i), i);
        ASSERT_EQ(hmC.get(i), i);
    }
}