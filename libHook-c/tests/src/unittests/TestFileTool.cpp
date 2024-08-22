/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/FileTool.h>

using namespace std;
using namespace scaler;



TEST(FileTool, findStrSplit) {

    //Normal case
    std::string str1 = "5560c7bd1000-5560c7bdd000 r--p 00000000 fd:01 8915156                    /home/st/Projects/scaler/cmake-build-debug/tests/TestLibHook";
    vector<ssize_t> splitIndexes = findStrSplit(str1, ' ');
    //for (int i = 0; i < splitIndexes.size(); i += 2) {
    //    printf("%s\n", str1.substr(splitIndexes[i], splitIndexes[i + 1]-splitIndexes[i]).c_str());
    //}
    vector<ssize_t> expectedRlt1 = {0, 25, 26, 30, 31, 39, 40, 45, 46, 53, 73, 133};
    EXPECT_EQ(splitIndexes, expectedRlt1);

    //String ending with char
    std::string str2 = "7fd8cf719000-7fd8cf71d000 rw-p 00000000 00:00 0     ";
    splitIndexes = findStrSplit(str2, ' ');
    //for (int i = 0; i < splitIndexes.size(); i += 2) {
    //    printf("%s\n", str2.substr(splitIndexes[i], splitIndexes[i + 1]-splitIndexes[i]).c_str());
    //}
    vector<ssize_t> expectedRlt2 = {0, 25, 26, 30, 31, 39, 40, 45, 46, 47};
    EXPECT_EQ(splitIndexes, expectedRlt2);

    //Stirng purely consist of splitchar
    std::string str3 = "      ";
    splitIndexes = findStrSplit(str3, ' ');
    EXPECT_TRUE(splitIndexes.size() == 0);

    //Empty string
    std::string str4 = "";
    splitIndexes = findStrSplit(str4, ' ');
    EXPECT_TRUE(splitIndexes.size() == 0);

}

TEST(FileTool, findLibName) {


}
