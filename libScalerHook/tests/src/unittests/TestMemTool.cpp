#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/MemTool.h>
#include <util/tool/MemoryTool_Linux.h>

using namespace std;
using namespace scaler;

TEST(MemTool, binCodeSearch) {

    MemoryTool_Linux *memTool = MemoryTool_Linux::getInst();
    char tgt1[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    char kwd1[] = {3, 4, 5};

    char *startAddr = static_cast<char *>(memTool->binCodeSearch(tgt1, 10, kwd1, 3));
    EXPECT_EQ(startAddr[0], 3);
    EXPECT_EQ(startAddr[1], 4);
    EXPECT_EQ(startAddr[2], 5);

    char tgt2[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    char kwd2[] = {9, 10, 11};
    startAddr = static_cast<char *>(memTool->binCodeSearch(tgt2, 10, kwd2, 3));
    EXPECT_EQ(startAddr, nullptr);

    char tgt3[] = {1, 2, 3, 4};
    char kwd3[] = {1, 2, 3, 4};
    startAddr = static_cast<char *>(memTool->binCodeSearch(tgt3, 4, kwd3, 4));
    EXPECT_EQ(startAddr, tgt3);

}