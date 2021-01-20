#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/FileTool.h>

using namespace std;
using namespace scaler;

TEST(FileTool, binCodeSearch) {
    char tgt1[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    char kwd1[] = {3, 4, 5};

    char *startAddr = static_cast<char *>(binCodeSearch(tgt1, 10, kwd1, 3));
    EXPECT_EQ(startAddr[0], 3);
    EXPECT_EQ(startAddr[1], 4);
    EXPECT_EQ(startAddr[2], 5);

    char tgt2[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    char kwd2[] = {9, 10, 11};
    startAddr = static_cast<char *>(binCodeSearch(tgt2, 10, kwd2, 3));
    EXPECT_EQ(startAddr, nullptr);

    char tgt3[] = {1, 2, 3, 4};
    char kwd3[] = {1, 2, 3, 4};
    startAddr = static_cast<char *>(binCodeSearch(tgt3, 4, kwd3, 4));
    EXPECT_EQ(startAddr, tgt3);

}

TEST(FileTool, findStrSplit) {
    //Normal case
    std::string str1 = "5560c7bd1000-5560c7bdd000 r--p 00000000 fd:01 8915156                    /home/st/Projects/Scaler/cmake-build-debug/tests/TestLibHook";
    vector<size_t> splitIndexes = findStrSplit(str1, ' ');
    //for (int i = 0; i < splitIndexes.size(); i += 2) {
    //    printf("%s\n", str1.substr(splitIndexes[i], splitIndexes[i + 1]-splitIndexes[i]).c_str());
    //}
    vector<size_t> expectedRlt1 = {0, 25, 26, 30, 31, 39, 40, 45, 46, 53, 73, 133};
    EXPECT_EQ(splitIndexes, expectedRlt1);

    //String ending with char
    std::string str2 = "7fd8cf719000-7fd8cf71d000 rw-p 00000000 00:00 0     ";
    splitIndexes = findStrSplit(str2, ' ');
    //for (int i = 0; i < splitIndexes.size(); i += 2) {
    //    printf("%s\n", str2.substr(splitIndexes[i], splitIndexes[i + 1]-splitIndexes[i]).c_str());
    //}
    vector<size_t> expectedRlt2 = {0, 25, 26, 30, 31, 39, 40, 45, 46, 47};
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
