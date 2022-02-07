#include <gtest/gtest.h>
#include <util/datastructure/List.h>

using namespace scaler;

TEST(List, isEmpty) {
    List<int> myList;
    ASSERT_TRUE(myList.isEmpty());
    ASSERT_TRUE(myList.head.next == &myList.tail);
    ASSERT_TRUE(myList.tail.prev == &myList.head);
    ASSERT_EQ(myList.getSize(), 0);

    myList.insertAfter(myList.getHead(), 0);
    ASSERT_EQ(myList.getSize(), 1);
    ASSERT_FALSE(myList.isEmpty());
    ASSERT_FALSE(myList.head.next == &myList.tail);
    ASSERT_FALSE(myList.tail.prev == &myList.head);
}

TEST(List, insert) {
    List<int> myList;
    //Insert after nullptr
    ASSERT_DEATH(myList.insertAfter(nullptr, 0), ".*node != nullptr.*");

    //Insert after tail
    ASSERT_DEATH(myList.insertAfter(myList.getTail(), 0), ".*node != &tail.*");

    //Insert at top
    for (int i = 1; i <= 5; ++i)
        myList.insertAfter(myList.getHead(), i);
    ASSERT_EQ(myList.getSize(), 5);

    ListEntry<int> *curEntry = myList.getHead();
    for (int i = 5; i >= 1; --i) {
        curEntry = curEntry->getNext();
        ASSERT_EQ(curEntry->getVal(), i);
    }

    //Pushback
    for (int i = 1; i <= 5; ++i)
        myList.pushBack(i);

    for (int i = 1; i <= 5; ++i) {
        curEntry = curEntry->getNext();
        ASSERT_EQ(curEntry->getVal(), i);
    }
    curEntry = curEntry->getNext();
    ASSERT_EQ(curEntry, myList.getTail());



    //Insert in the middle
    myList.insertAfter(curEntry->getPrev()->getPrev()->getPrev()->getPrev()->getPrev()->getPrev()->getPrev(), 0);

    curEntry = myList.getHead();
    for (int i = 6; i <= 0; ++i) {
        curEntry = curEntry->getNext();
        ASSERT_EQ(i, curEntry->getVal());
    }
}


TEST(List, erase) {
    List<int> myList;

    //Erase nullptr
    ASSERT_DEATH(myList.erase(nullptr), ".*node != nullptr.*");

    //Erase head
    ASSERT_DEATH(myList.erase(myList.getHead()), ".*node != &head.*");
    //Erase tail
    ASSERT_DEATH(myList.erase(myList.getTail()), ".*node != &tail.*");

    //Erase one element
    myList.pushBack(0);
    myList.erase(myList.getHead()->getNext());
    ASSERT_TRUE(myList.isEmpty());

    //Erase multiple lements
    for (int i = 1; i <= 5; ++i)
        myList.pushBack(i);
    ListEntry<int> *curEntry = myList.getHead()->getNext();
    for (int i = 1; i <= 5; ++i) {
        auto nextPtr = curEntry->getNext();
        if (i % 2 == 0)
            myList.erase(curEntry);
        curEntry = nextPtr;
    }
    ASSERT_EQ(myList.getSize(), 3);
    curEntry = myList.getHead();
    for (int i = 1; i <= 5; i += 2) {
        curEntry = curEntry->getNext();
        ASSERT_EQ(i, curEntry->getVal());
    }

}

TEST(List, copyconstruct) {
    List<int> myList;
    for (int i = 1; i <= 5; ++i)
        myList.pushBack(i);

    ListEntry<int> *curEntry = myList.getHead();
    for (int i = 1; i <= 5; ++i) {
        curEntry = curEntry->getNext();
        ASSERT_EQ(curEntry->getVal(), i);
    }

    List<int> myList1(myList);
    myList1.erase(myList1.getHead()->getNext());
    curEntry = myList1.getHead();
    for (int i = 2; i <= 5; ++i) {
        curEntry = curEntry->getNext();
        ASSERT_EQ(curEntry->getVal(), i);
    }


    List<int> myList2 = myList;
    myList2.pushBack(6);
    curEntry = myList2.getHead();
    for (int i = 1; i <= 6; ++i) {
        curEntry = curEntry->getNext();
        ASSERT_EQ(curEntry->getVal(), i);
    }

    //copy self
    myList2 = myList2;
    curEntry = myList2.getHead();
    for (int i = 1; i <= 6; ++i) {
        curEntry = curEntry->getNext();
        ASSERT_EQ(curEntry->getVal(), i);
    }

}

TEST(List, Iteration) {
    List<int> myList;
    ASSERT_TRUE(myList.isEmpty());
    auto beg = myList.begin();
    auto end = myList.end();
    ASSERT_TRUE(beg == beg);
    ASSERT_TRUE(end == end);
    ASSERT_TRUE(beg == end);

    for (int i = 1; i <= 7; ++i)
        myList.pushBack(i);

    ASSERT_TRUE(myList.begin() != myList.end());

    int index = 1;
    for (auto elem = myList.begin(); elem != myList.end(); ++elem) {
        ASSERT_EQ(*elem, index++);
    }
    index = 1;

    ListEntry<int> *curEntry = myList.getHead()->getNext();
    for (int i = 1; i <= 7; ++i) {
        auto nextPtr = curEntry->getNext();
        if (curEntry->getVal() % 2 == 0) {
            myList.erase(curEntry);
        }
        curEntry = nextPtr;
    }
    ASSERT_EQ(myList.getSize(), 4);
    auto begin = myList.rbegin();
    ASSERT_EQ(*begin, 7);
    EXPECT_FALSE(begin == myList.rend());
    --begin;
    ASSERT_EQ(*begin, 5);
    EXPECT_FALSE(begin == myList.rend());
    begin--;
    ASSERT_EQ(*begin, 3);
    EXPECT_FALSE(begin == myList.rend());
    begin--;
    ASSERT_EQ(*begin, 1);
    --begin;
    EXPECT_TRUE(begin == myList.rend());


}
