#include <gtest/gtest.h>
#include <util/datastructure/Vector.h>

using namespace scaler;

TEST(Vector, insert) {
    Vector<int> myVec;
    //Insert after nullptr
    ASSERT_DEATH(myVec.insertAt(-1,0), ".*node != nullptr.*");

//    //Insert after tail
//    ASSERT_DEATH(myVec.insertAfter(myVec.getTail(), 0), ".*node != &tail.*");
//
//    //Insert at top
//    for (int i = 1; i <= 5; ++i)
//        myVec.insertAfter(myVec.getHead(), i);
//    ASSERT_EQ(myVec.getSize(), 5);
//
//    ListEntry<int> *curEntry = myVec.getHead();
//    for (int i = 5; i >= 1; --i) {
//        curEntry = curEntry->getNext();
//        ASSERT_EQ(curEntry->getVal(), i);
//    }
//
//    //Pushback
//    for (int i = 1; i <= 5; ++i)
//        myVec.pushBack(i);
//
//    for (int i = 1; i <= 5; ++i) {
//        curEntry = curEntry->getNext();
//        ASSERT_EQ(curEntry->getVal(), i);
//    }
//    curEntry = curEntry->getNext();
//    ASSERT_EQ(curEntry, myVec.getTail());
//
//
//
//    //Insert in the middle
//    myVec.insertAfter(curEntry->getPrev()->getPrev()->getPrev()->getPrev()->getPrev()->getPrev()->getPrev(), 0);
//
//    curEntry = myVec.getHead();
//    for (int i = 6; i <= 0; ++i) {
//        curEntry = curEntry->getNext();
//        ASSERT_EQ(i, curEntry->getVal());
//    }
}