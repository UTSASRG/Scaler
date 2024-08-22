/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
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