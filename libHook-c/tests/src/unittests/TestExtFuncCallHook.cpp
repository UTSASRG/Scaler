/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <iostream>

#include <gtest/gtest.h>
#include <plthook.h>
#include <util/tool/ProcInfoParser.h>

using namespace std;
using namespace scaler;

//Reference
extern char __startplt, __endplt, __startpltgot, __endpltgot, __startpltsec, __endpltsec;

TEST(ExtFuncCallHook, locSecAndSegInMem) {

//    plthook_t *myPltHook;
////Find plthook
//    plthook_open(&myPltHook,
//                 NULL);
//
//
//    vector<string> funcNameArr;
//    vector<void *> addrArr;
//
//    unsigned int pos = 0;
//    const char *name;
//    void **addr;
//    int i;
//    while (
//            plthook_enum(myPltHook, &pos, &name, &addr
//            ) == 0) {
////printf("   %s\n", name);
//        funcNameArr.
//                emplace_back(std::string(name)
//        );
//        addrArr.
//                emplace_back(addr);
//    }
//
//    ExtFuncCallHookAsm *hook = ExtFuncCallHookAsm::getInst();
//    hook->
//
//            locateRequiredSecAndSeg();
//
//
//    scaler::ExtFuncCallHookAsm::ELFImgInfo curElfImgInfo = hook->elfImgInfoMap[
//            hook->pmParser.fileIDMap[hook->pmParser.curExecAbsolutePath]];
//
//
//    EXPECT_EQ(curElfImgInfo.pltStartAddr, &__startplt);
//    EXPECT_EQ(curElfImgInfo.pltEndAddr, &__endplt);
//    EXPECT_EQ(curElfImgInfo.pltSecStartAddr, &__startpltsec);
////    EXPECT_EQ(curElfImgInfo.pltSecEndAddr, &__endpltsec);
////    EXPECT_EQ(curElfImgInfo._DYNAMICAddr, _DYNAMIC);
//    for (auto &curSym: hook->allExtSymbol) {
//        EXPECT_EQ(curSym.symbolName, funcNameArr[curSym.symIdInFile]);
//        EXPECT_EQ(curSym.gotEntry, addrArr[i]);
//    }


}

TEST(ExtFuncCallHook, install) {
//    ExtFuncCallHook *hook = ExtFuncCallHook::getInst();
//    hook->install();

}

void *addr1;


#include <FuncWithDiffParms.h>
#include <TenThousandFunc.h>

TEST(ExtFuncCallHook, compareAddressAndFuncName) {
//    resolveSystemFunc();
//
//    plthook_t *myPltHook;
//    //Find plthook
//    plthook_open(&myPltHook, NULL);
//
//
//    vector<string> funcNameArr;
//    vector<void *> addrArr;
//
//    unsigned int pos = 0;
//    const char *name;
//    void **addr;
//    int i;
//    while (plthook_enum(myPltHook, &pos, &name, &addr) == 0) {
//        //printf("   %s\n", name);
//        funcNameArr.emplace_back(std::string(name));
//        addrArr.emplace_back(addr);
//    }
//
//    ExtFuncCallHookAsm *hook = ExtFuncCallHookAsm::getInst();
//    //todo:fix
//    //hook->install();
//
//    scaler::ExtFuncCallHookAsm::ELFImgInfo curElfImgInfo = hook->elfImgInfoMap[
//            hook->pmParser.fileIDMap[hook->pmParser.curExecAbsolutePath]];
//
//
//    EXPECT_EQ(curElfImgInfo.pltStartAddr, &__startplt);
//    EXPECT_EQ(curElfImgInfo.pltEndAddr, &__endplt);
//    EXPECT_EQ(curElfImgInfo.pltSecStartAddr, &__startpltsec);
////    EXPECT_EQ(curElfImgInfo.pltSecEndAddr, &__endpltsec);
////    EXPECT_EQ(curElfImgInfo._DYNAMICAddr, _DYNAMIC);
//
//
//    for (auto& curSym:hook->allExtSymbol) {
//        EXPECT_EQ(curSym.symbolName, funcNameArr[i]);
//        EXPECT_EQ(curSym.gotEntry, addrArr[i]);
//    }
//
//
////printf("FuncA: %p\n", getFuncAddr("funcA"));
////todo: relative path
//
//    //scaler::ExtFuncCallHookAsm::ELFImgInfo &libTestELFInfo = hook->elfImgInfoMap[hook->pmParser.fileIDMap.at(
//    //        "/home/st/Projects/scaler/cmake-build-debug/tests/libFuncCallTest.so")];
//    //auto systemFuncId = libTestELFInfo.funcIdMap.at("system");
//    //void **gotTableAddr = libTestELFInfo.allExtSymbol[systemFuncId].gotEntry;
//
//   // EXPECT_EQ(*gotTableAddr,
//    //          (void *) system);

}


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

#ifdef __AVX__
    structparm s;
    int e, f, g, h, i, j, k;
    long double ld;
    double m, n;
    __m256 y;
    __m512 z;
    funcEverything(e, f, s, g, h, ld, m, y, z, n, i, j, k);
#endif
    auto a1 = EXTVAR_VAR1 + 1;
    a1 += A::asdf;
    a1 += EXTVAR_VAR2;
    a1 += SELF_EXTERN;
    return 0;
}

