#include <gtest/gtest.h>
#include <util/datastructure/Hashmap.hh>
#include <util/hook/ExtFuncCallHook_Linux.hh>

using namespace scaler;

TEST(ELFImageInfo, copy) {

    ExtFuncCallHook_Linux::ExtSymInfo testSymInfo;
    testSymInfo.symbolName = "symbol1";
    testSymInfo.type = 15;
    int testInt;
    void *testPtr = &testInt;
    ExtFuncCallHook_Linux::ELFImgInfo elfImgInfo1;
    elfImgInfo1.filePath = "123";
    elfImgInfo1.pltStartAddr = testPtr;
    elfImgInfo1._DYNAMICAddr = static_cast<Elf64_Dyn *>(testPtr);
    elfImgInfo1.realAddrResolved.insertAt(0, true);
    elfImgInfo1.realAddrResolved.insertAt(1, false);
    elfImgInfo1.hookedExtSymbol.put(1, testSymInfo);
    elfImgInfo1.oriPltCode.put(1, (void *) &testInt);
    elfImgInfo1.idFuncMap[0] = "symbol1";

    ExtFuncCallHook_Linux::ExtSymInfo testSymInfoCp = testSymInfo;
    ASSERT_EQ(testSymInfoCp.symbolName, "symbol1");
    ASSERT_EQ(testSymInfoCp.type, 15);
    ASSERT_EQ(testSymInfoCp.type, 15);

    ExtFuncCallHook_Linux::ELFImgInfo elfImgInfo1cp = elfImgInfo1;


    ASSERT_EQ(testSymInfoCp.type, 15);
    ASSERT_EQ(elfImgInfo1cp.filePath, "123");
    ASSERT_EQ(elfImgInfo1cp.pltStartAddr, testPtr);
    ASSERT_EQ(elfImgInfo1cp._DYNAMICAddr, static_cast<Elf64_Dyn *>(testPtr));
    ASSERT_EQ(elfImgInfo1cp.realAddrResolved[0], true);
    ASSERT_EQ(elfImgInfo1cp.realAddrResolved[1], false);
    ASSERT_EQ(elfImgInfo1cp.hookedExtSymbol.get(1), testSymInfoCp);
    ASSERT_EQ(elfImgInfo1cp.oriPltCode.get(1), &testInt);
    ASSERT_EQ(elfImgInfo1cp.idFuncMap[0], "symbol1");
}