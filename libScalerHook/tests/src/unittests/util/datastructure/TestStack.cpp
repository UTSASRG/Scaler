#include <gtest/gtest.h>
#include <util/datastructure/Stack.h>

TEST(Stack, PushPop) {
    scaler::Stack<int> myStack(5);
    myStack.push(1);
    myStack.push(2);
    myStack.push(3);
    myStack.push(4);
    myStack.push(5);

    ASSERT_EQ(myStack.peek(), 5);
    myStack.pop();
    ASSERT_EQ(myStack.peek(), 4);
    myStack.pop();
    ASSERT_EQ(myStack.peek(), 3);
    myStack.pop();
    ASSERT_EQ(myStack.peek(), 2);
    myStack.pop();
    ASSERT_EQ(myStack.peek(), 1);
    myStack.pop();
}

TEST(Stack, StackCopy) {
    auto* myStack=new scaler::Stack<int>(5);
    myStack->push(1);
    myStack->push(2);
    myStack->push(3);
    myStack->push(4);
    myStack->push(5);

    scaler::Stack<int> myStack21=*myStack;

    ASSERT_EQ(myStack->peek(),5);
    myStack->pop();
    ASSERT_EQ(myStack->peek(),4);
    myStack->pop();
    ASSERT_EQ(myStack->peek(),3);
    myStack->pop();
    ASSERT_EQ(myStack->peek(),2);
    myStack->pop();
    ASSERT_EQ(myStack->peek(),1);
    myStack->pop();
    delete myStack;

    ASSERT_EQ(myStack21.peekpop(),5);
    ASSERT_EQ(myStack21.peekpop(),4);
    ASSERT_EQ(myStack21.peekpop(),3);
    ASSERT_EQ(myStack21.peekpop(),2);
    ASSERT_EQ(myStack21.peekpop(),1);
}