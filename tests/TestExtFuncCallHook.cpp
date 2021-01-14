#include <iostream>

#include <gtest/gtest.h>
#include <util/hook/ExtFuncCallHook.hh>

using namespace std;
using namespace scaler;

//Reference
extern char __startplt, __endplt, __startpltgot, __endpltgot, __startpltsec, __endpltsec;

TEST(ExtFuncCallHook, locSectionInMem) {
    PMParser parser;
    parser.parsePMMap();

    ExtFuncCallHook hook;
    hook.locSectionInMem();

    auto &calcPltPtr = hook.fileSecMap[hook.symbolNames[parser.curExecFileName]][".plt"];
    auto &calcGotPtr = hook.fileSecMap[hook.symbolNames[parser.curExecFileName]][".plt.got"];
    auto &calcPltSecPtr = hook.fileSecMap[hook.symbolNames[parser.curExecFileName]][".plt.sec"];

    EXPECT_EQ(calcPltPtr.startAddr, &__startplt);
    EXPECT_EQ(calcPltPtr.endAddr, &__endplt);
    EXPECT_EQ(calcGotPtr.startAddr, &__startpltgot);
    EXPECT_EQ(calcGotPtr.endAddr, &__endpltgot);
    EXPECT_EQ(calcPltSecPtr.startAddr, &__startpltsec);
    EXPECT_EQ(calcPltSecPtr.endAddr, &__endpltsec);
}

TEST(ExtFuncCallHook, install) {
    ExtFuncCallHook hook;
    hook.install();

}

