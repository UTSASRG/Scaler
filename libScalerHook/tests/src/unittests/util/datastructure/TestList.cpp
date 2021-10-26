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

TEST(List, insertAfter_pushBack) {
    //Insert at top
    List<int> myList;
    for (int i = 1; i <= 5; ++i)
        myList.insertAfter(myList.getHead(), i);
    ASSERT_EQ(myList.getSize(), 5);

    ListEntry<int> *newEntry = myList.getHead();
    for (int i = 5; i >= 1; --i) {
        newEntry = newEntry->getNext();
        ASSERT_EQ(newEntry->getVal(), i);
    }

    for (int i = 1; i <= 5; ++i)
        myList.pushBack(i);

    for (int i = 1; i <= 5; ++i) {
        newEntry = newEntry->getNext();
        ASSERT_EQ(newEntry->getVal(), i);
    }
    newEntry = newEntry->getNext();
    ASSERT_EQ(newEntry, myList.getTail());
}
