#include <gtest/gtest.h>
#include <util/datastructure/Vector.h>

using namespace scaler;

TEST(Vector, insert) {
    Vector<int> myVec;
    //Insert after nullptr
    ASSERT_DEATH(myVec.insertAt(-1, 0), ".*0 <= index && index <= size.*");

    //Insert at top
    for (int i = 1; i <= 5; ++i)
        myVec.insertAt(0, i);
    ASSERT_EQ(myVec.getSize(), 5);

    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(myVec[i], 5 - i);
    }
    ASSERT_EQ(myVec.getSize(), 5);


    myVec.clear();

    ASSERT_EQ(myVec.getSize(), 0);

    //Pushback
    for (int i = 1; i <= 5; ++i)
        myVec.pushBack(i);

    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(myVec[i], i + 1);
    }

    ASSERT_EQ(myVec.getSize(), 5);

    myVec.clear();
    ASSERT_EQ(myVec.getSize(), 0);


}


TEST(Vector, erase) {
    Vector<int> myVec;

    //Erase nullptr
    ASSERT_DEATH(myVec.erase(-1), ".*0 <= index && index < size.*");
    ASSERT_DEATH(myVec.erase(0), ".*0 <= index && index < size.*");


    //Erase one element
    myVec.pushBack(0);
    myVec.erase(myVec.getSize() - 1);
    ASSERT_TRUE(myVec.isEmpty());

    //Erase multiple lements
    for (int i = 1; i <= 5; ++i)
        myVec.pushBack(i);
    for (int i = 4; i >= 0; --i) {
        if (myVec[i] % 2 == 0){
            myVec.erase(i);
        }
    }
    ASSERT_EQ(myVec.getSize(), 3);

    for (int i = 1; i <= 5; i += 2) {
        ASSERT_EQ(i, myVec[i / 2]);
    }

}

TEST(Vector, getVal) {
    Vector<int> myVec;

    for (int i = 1; i <= 5; ++i)
        myVec.pushBack(i);
    ASSERT_EQ(myVec[2], 3);
    ASSERT_DEATH(myVec[6], ".*0 <= index && index < size.*");

}

TEST(Vector, Iteration) {
    Vector<int> myVec;
    ASSERT_TRUE(myVec.isEmpty());
    auto beg = myVec.begin();
    auto end = myVec.end();
    ASSERT_TRUE(beg == beg);
    ASSERT_TRUE(end == end);
    ASSERT_TRUE(beg == end);

    for (int i = 1; i <= 7; ++i)
        myVec.pushBack(i);

    ASSERT_TRUE(myVec.begin() != myVec.end());

    int index = 1;
    for (auto elem = myVec.begin(); elem != myVec.end(); ++elem) {
        ASSERT_EQ(*elem, index++);
    }
    index = 1;

    for (int i =  myVec.getSize()-1; i >= 0; --i) {
        if (myVec[i] % 2 == 0){
            myVec.erase(i);
        }
    }
    ASSERT_EQ(myVec.getSize(), 4);
    auto rbegin = myVec.rbegin();
    ASSERT_EQ(*rbegin, 7);
    EXPECT_FALSE(rbegin == myVec.rend());
    --rbegin;
    ASSERT_EQ(*rbegin, 5);
    EXPECT_FALSE(rbegin == myVec.rend());
    rbegin--;
    ASSERT_EQ(*rbegin, 3);
    EXPECT_FALSE(rbegin == myVec.rend());
    rbegin--;
    ASSERT_EQ(*rbegin, 1);
    --rbegin;
    EXPECT_TRUE(rbegin == myVec.rend());

}

TEST(Vector, copyconstruct) {
    Vector<int> myVec;
    for (int i = 1; i <= 5; ++i)
        myVec.pushBack(i);

    for (int i = 1; i <= 5; ++i) {
        ASSERT_EQ(myVec[i-1], i);
    }

    Vector<int> myVec1(myVec);
    for (int i = 1; i <= 5; ++i) {
        ASSERT_EQ(myVec1[i-1], i);
    }


    Vector<int> myVec2 = myVec;
    myVec2.pushBack(6);
    for (int i = 0; i < 6; ++i) {
        ASSERT_EQ(myVec2[i], i+1);
    }

    //copy self
    myVec2 = myVec2;
    for (int i = 0; i < 6; ++i) {
        ASSERT_EQ(myVec2[i], i+1);
    }
}