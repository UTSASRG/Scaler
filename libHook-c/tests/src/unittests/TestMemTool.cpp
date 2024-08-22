/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/MemoryTool.h>

using namespace std;
using namespace scaler;

TEST(MemTool, memSearch) {

    char tgt1[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    char kwd1[] = {3, 4, 5};

    char *startAddr = static_cast<char *>(memSearch(tgt1, 10, kwd1, 3));
    EXPECT_EQ(startAddr[0], 3);
    EXPECT_EQ(startAddr[1], 4);
    EXPECT_EQ(startAddr[2], 5);

    char tgt2[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    char kwd2[] = {9, 10, 11};
    startAddr = static_cast<char *>(memSearch(tgt2, 10, kwd2, 3));
    EXPECT_EQ(startAddr, nullptr);

    char tgt3[] = {1, 2, 3, 4};
    char kwd3[] = {1, 2, 3, 4};
    startAddr = static_cast<char *>(memSearch(tgt3, 4, kwd3, 4));
    EXPECT_EQ(startAddr, tgt3);

}