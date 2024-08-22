/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
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

