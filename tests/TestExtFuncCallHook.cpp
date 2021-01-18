#include <iostream>

#include <gtest/gtest.h>
#include <util/hook/ExtFuncCallHook.hh>
#include <plthook.h>

using namespace std;
using namespace scaler;

//Reference
extern char __startplt, __endplt, __startpltgot, __endpltgot, __startpltsec, __endpltsec;

TEST(ExtFuncCallHook, locSecAndSegInMem) {
    PMParser parser;
    parser.parsePMMap();

    ExtFuncCallHook *hook = ExtFuncCallHook::getInst();
    hook->locSecAndSegInMem();

    auto &curFileSecInfo = hook->fileSecInfoMap.at(hook->fileIDMap.at(parser.curExecFileName));
    auto &calcPltPtr = curFileSecInfo.at(SEC_NAME::PLT);
    auto &calcGotPtr = curFileSecInfo.at(SEC_NAME::GOT);
    auto &calcPltSecPtr = curFileSecInfo.at(SEC_NAME::PLT_SEC);

    EXPECT_EQ(calcPltPtr.startAddr, &__startplt);
    EXPECT_EQ(calcPltPtr.endAddr, &__endplt);
    EXPECT_EQ(calcGotPtr.startAddr, &__startpltgot);
    EXPECT_EQ(calcGotPtr.endAddr, &__endpltgot);
    EXPECT_EQ(calcPltSecPtr.startAddr, &__startpltsec);
    EXPECT_EQ(calcPltSecPtr.endAddr, &__endpltsec);

    auto &curFileSegInfo = hook->fileSegInfoMap.at(hook->fileIDMap.at(parser.curExecFileName));
    auto &calcDynamicSegPtr = curFileSegInfo.at(PT_DYNAMIC);

    printf("_DYNAMIC file:%p ptr:%p\n", calcDynamicSegPtr.startAddr, _DYNAMIC);

    EXPECT_EQ(calcDynamicSegPtr.startAddr, _DYNAMIC);
}

TEST(ExtFuncCallHook, install) {
    ExtFuncCallHook *hook = ExtFuncCallHook::getInst();
    hook->install();

}

void *addr1;

TEST(ExtFuncCallHook, findExecNameByAddr) {
    //Get current executable file name
    PMParser parser;
    parser.parsePMMap();


    ExtFuncCallHook *hook = ExtFuncCallHook::getInst();

    hook->locSecAndSegInMem();
    size_t funcId = hook->findExecNameByAddr(addr1);
    EXPECT_EQ(parser.curExecFileName, hook->fileLoadMap[funcId].fileName);

    void *funcPtr = (void *) printf;
    funcId = hook->findExecNameByAddr(funcPtr);
    auto &execName = hook->fileLoadMap[funcId].fileName;

    EXPECT_TRUE(execName.find("libc") != std::string::npos);

    hook->fileLoadMap.clear();
    for (int i = 0; i < 4; i += 2) {
        LoadingInfo newEntry;
        newEntry.startAddr = (void *) i;
        hook->fileLoadMap.emplace_back(newEntry);
    }
    funcId = hook->findExecNameByAddr((void *) 0);
    EXPECT_EQ(funcId, 0);
    funcId = hook->findExecNameByAddr((void *) 1);
    EXPECT_EQ(funcId, 0);
    funcId = hook->findExecNameByAddr((void *) 2);
    EXPECT_EQ(funcId, 2);
    funcId = hook->findExecNameByAddr((void *) 3);
    EXPECT_EQ(funcId, 2);

}

#include <FuncWithDiffParms.h>
#include <TenThousandFunc.h>


extern int EXTVAR_VAR1;

static int EXTVAR_VAR2;

extern int SELF_EXTERN;
int SELF_EXTERN = 0;


int nativeFunc() {
    funcA();
    funcB(1);
    funcC(1, 2);
    funcD(1, 2, 3);
    funcE(1, 2, 3);
    structparm strP;

    structparm s;
    int e, f, g, h, i, j, k;
    long double ld;
    double m, n;
    __m256 y;
    __m512 z;
    funcEverything(e, f, s, g, h, ld, m, y, z, n, i, j, k);

    auto a1 = EXTVAR_VAR1 + 1;
    a1 += A::asdf;
    a1 += EXTVAR_VAR2;
    a1 += SELF_EXTERN;

}

