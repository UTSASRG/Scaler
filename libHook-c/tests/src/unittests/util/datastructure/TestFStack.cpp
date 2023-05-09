#include <gtest/gtest.h>
#include <util/datastructure/FStack.h>

TEST(FStack, unsafePushPop) {
    scaler::FStack<int, 4> myStack;
    ASSERT_TRUE(myStack.isEmpty());

    myStack.unsafePush(1);
    ASSERT_EQ(myStack.getSize(), 1);
    myStack.unsafePush(2);
    ASSERT_EQ(myStack.getSize(), 2);
    myStack.unsafePush(3);
    ASSERT_EQ(myStack.getSize(), 3);
    myStack.unsafePush(4);
    ASSERT_EQ(myStack.getSize(), 4);

    ASSERT_EQ(myStack.unsafePeek(), 4);
    ASSERT_EQ(myStack.getSize(), 4);
    myStack.unsafePop();
    ASSERT_EQ(myStack.unsafePeek(), 3);
    ASSERT_EQ(myStack.getSize(), 3);
    myStack.unsafePop();
    ASSERT_EQ(myStack.unsafePeek(), 2);
    ASSERT_EQ(myStack.getSize(), 2);
    myStack.unsafePop();
    ASSERT_EQ(myStack.unsafePeek(), 1);
    ASSERT_EQ(myStack.getSize(), 1);
    myStack.unsafePop();
    ASSERT_EQ(myStack.getSize(), 0);

    ASSERT_TRUE(myStack.isEmpty());
}

TEST(FStack, safePushPeekPop) {
    scaler::FStack<int, 4> myStack;
    ASSERT_TRUE(myStack.isEmpty());

    ASSERT_TRUE(myStack.safePush(1));
    ASSERT_EQ(myStack.getSize(), 1);
    ASSERT_TRUE(myStack.safePush(2));
    ASSERT_EQ(myStack.getSize(), 2);
    ASSERT_TRUE(myStack.safePush(3));
    ASSERT_EQ(myStack.getSize(), 3);
    ASSERT_TRUE(myStack.safePush(4));
    ASSERT_EQ(myStack.getSize(), 4);
    ASSERT_FALSE(myStack.safePush(4));
    ASSERT_EQ(myStack.getSize(), 4);

    ASSERT_EQ(myStack.unsafePeek(), 4);
    ASSERT_EQ(myStack.getSize(), 4);
    ASSERT_TRUE(myStack.safepop());

    ASSERT_EQ(myStack.unsafePeek(), 3);
    ASSERT_EQ(myStack.getSize(), 3);
    ASSERT_TRUE(myStack.safepop());

    ASSERT_EQ(myStack.unsafePeek(), 2);
    ASSERT_EQ(myStack.getSize(), 2);
    ASSERT_TRUE(myStack.safepop());

    ASSERT_EQ(myStack.unsafePeek(), 1);
    ASSERT_EQ(myStack.getSize(), 1);
    ASSERT_TRUE(myStack.safepop());

    ASSERT_FALSE(myStack.safepop());
    ASSERT_EQ(myStack.getSize(), 0);

    ASSERT_TRUE(myStack.isEmpty());
}

TEST(FStack, unsafePeekPop) {
    scaler::FStack<int, 4> myStack;
    ASSERT_TRUE(myStack.isEmpty());

    myStack.unsafePush(1);
    ASSERT_EQ(myStack.getSize(), 1);
    myStack.unsafePush(2);
    ASSERT_EQ(myStack.getSize(), 2);
    myStack.unsafePush(3);
    ASSERT_EQ(myStack.getSize(), 3);
    myStack.unsafePush(4);
    ASSERT_EQ(myStack.getSize(), 4);

    ASSERT_EQ(myStack.unsafePeekpop(), 4);
    ASSERT_EQ(myStack.getSize(), 3);
    ASSERT_EQ(myStack.unsafePeekpop(), 3);
    ASSERT_EQ(myStack.getSize(), 2);
    ASSERT_EQ(myStack.unsafePeekpop(), 2);
    ASSERT_EQ(myStack.getSize(), 1);
    ASSERT_EQ(myStack.unsafePeekpop(), 1);
    ASSERT_EQ(myStack.getSize(), 0);
    ASSERT_TRUE(myStack.isEmpty());
}

TEST(FStack, safePeekPop) {
    scaler::FStack<int, 4> myStack;
    ASSERT_TRUE(myStack.isEmpty());

    myStack.safePush(1);
    ASSERT_EQ(myStack.getSize(), 1);
    myStack.unsafePush(2);
    ASSERT_EQ(myStack.getSize(), 2);
    myStack.unsafePush(3);
    ASSERT_EQ(myStack.getSize(), 3);
    myStack.unsafePush(4);
    ASSERT_EQ(myStack.getSize(), 4);

    const int *rlt = nullptr;
    ASSERT_EQ(myStack.getSize(), 4);
    ASSERT_TRUE(myStack.safePeekpop(rlt));
    ASSERT_EQ(*rlt, 4);

    rlt = nullptr;
    ASSERT_EQ(myStack.getSize(), 3);
    ASSERT_TRUE(myStack.safePeekpop(rlt));
    ASSERT_EQ(*rlt, 3);

    rlt = nullptr;
    ASSERT_EQ(myStack.getSize(), 2);
    ASSERT_TRUE(myStack.safePeekpop(rlt));
    ASSERT_EQ(*rlt, 2);

    rlt = nullptr;
    ASSERT_EQ(myStack.getSize(), 1);
    ASSERT_TRUE(myStack.safePeekpop(rlt));
    ASSERT_EQ(*rlt, 1);
    ASSERT_EQ(myStack.getSize(), 0);
    ASSERT_TRUE(myStack.isEmpty());
}

TEST(FStack, StackCopy) {
    scaler::FStack<int, 4> myStack;
    myStack.safePush(1);
    myStack.safePush(2);
    myStack.safePush(3);
    myStack.safePush(4);

    scaler::FStack<int, 4> myStack1 = myStack;
    ASSERT_EQ(myStack1.getSize(), 4);
    ASSERT_TRUE(myStack1.safepop());
    ASSERT_EQ(myStack1.getSize(), 3);
    ASSERT_TRUE(myStack1.safepop());
    ASSERT_EQ(myStack1.getSize(), 2);
    ASSERT_TRUE(myStack1.safepop());
    ASSERT_EQ(myStack1.getSize(), 1);
    ASSERT_TRUE(myStack1.safepop());
    ASSERT_EQ(myStack1.getSize(), 0);

    const int *rlt = nullptr;
    ASSERT_EQ(myStack.getSize(), 4);
    ASSERT_TRUE(myStack.safePeekpop(rlt));
    ASSERT_EQ(*rlt, 4);

    rlt = nullptr;
    ASSERT_EQ(myStack.getSize(), 3);
    ASSERT_TRUE(myStack.safePeekpop(rlt));
    ASSERT_EQ(*rlt, 3);

    rlt = nullptr;
    ASSERT_EQ(myStack.getSize(), 2);
    ASSERT_TRUE(myStack.safePeekpop(rlt));
    ASSERT_EQ(*rlt, 2);

    rlt = nullptr;
    ASSERT_EQ(myStack.getSize(), 1);
    ASSERT_TRUE(myStack.safePeekpop(rlt));

    ASSERT_EQ(*rlt, 1);
    ASSERT_EQ(myStack.getSize(), 0);
    ASSERT_TRUE(myStack.isEmpty());

}