#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/FileTool.h>
#include <util/tool/StringTool.h>

using namespace std;
using namespace scaler;


TEST(StringTool, collapseStrSpace) {
    string out;
    collapseStrSpace(" ",out);
    EXPECT_TRUE(out.empty());
    collapseStrSpace("    ",out);
    EXPECT_TRUE(out.empty());
    collapseStrSpace("    1 23 4      5",out);
    EXPECT_TRUE(out=="1 23 4 5");

}