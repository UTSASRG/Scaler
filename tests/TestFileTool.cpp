#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/FileTool.h>

using namespace std;
using namespace scaler;

TEST(FileTool, binarySearch) {
    char arr1[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    char arr2[] = {3, 4, 5};

    char *startAddr = static_cast<char *>(binarySearch(arr1, 10, arr2, 3));
    EXPECT_EQ(startAddr[0], 3);
    EXPECT_EQ(startAddr[1], 4);
    EXPECT_EQ(startAddr[2], 5);

    char arr3[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    char arr4[] = {9, 10, 11};
    startAddr = static_cast<char *>(binarySearch(arr3, 10, arr4, 3));
    EXPECT_EQ(startAddr, nullptr);

}

TEST(FileTool, findStrSplit) {
    std::string str1 = "5560c7bd1000-5560c7bdd000 r--p 00000000 fd:01 8915156                    /home/st/Projects/Scaler/cmake-build-debug/tests/TestLibHook";
    vector<size_t> splitIndexes = findStrSplit(str1, ' ');
    for (int i = 0; i < splitIndexes.size(); i += 2) {
        printf("%s\n", str1.substr(splitIndexes[i], splitIndexes[i + 1]-splitIndexes[i]).c_str());
    }

    str1 = "7fd8cf719000-7fd8cf71d000 rw-p 00000000 00:00 0     ";
    splitIndexes = findStrSplit(str1, ' ');
    for (int i = 0; i < splitIndexes.size(); i += 2) {
        printf("%s\n", str1.substr(splitIndexes[i], splitIndexes[i + 1]-splitIndexes[i]).c_str());
    }
}
//int main() {
//
//    return 0;
//}
