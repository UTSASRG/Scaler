#include <gtest/gtest.h>
#include <util/datastructure/Array.h>

using namespace scaler;

TEST(Array, insert) {
    Array<int> myArray(30);

    *myArray.pushBack() = 0;
    *myArray.pushBack() = 1;
    *myArray.pushBack() = 2;
    *myArray.pushBack() = 3;
    ASSERT_EQ(myArray[0],0);
    ASSERT_EQ(myArray[1],1);
    ASSERT_EQ(myArray[2],2);
    ASSERT_EQ(myArray[3],3);
    ASSERT_EQ(myArray.getSize(),4);


    *myArray.insertAt(1) = 1;

    ASSERT_EQ(myArray[0],0);
    ASSERT_EQ(myArray[1],1);
    ASSERT_EQ(myArray[2],1);
    ASSERT_EQ(myArray[3],2);
    ASSERT_EQ(myArray[4],3);
    ASSERT_EQ(myArray.getSize(),5);

}

TEST(Array, delete) {
    Array<int> myArray(30);

    *myArray.pushBack() = 0;
    *myArray.pushBack() = 1;
    *myArray.pushBack() = 2;
    *myArray.pushBack() = 3;
    ASSERT_EQ(myArray[0],0);
    ASSERT_EQ(myArray[1],1);
    ASSERT_EQ(myArray[2],2);
    ASSERT_EQ(myArray[3],3);
    ASSERT_EQ(myArray.getSize(),4);

    myArray.erase(1);
    myArray.erase(1);

    ASSERT_EQ(myArray.getSize(),2);
    ASSERT_EQ(myArray[0],0);
    ASSERT_EQ(myArray[1],3);

    Array<Array<int>> myArray1(30);

}

