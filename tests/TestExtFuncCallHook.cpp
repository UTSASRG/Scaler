#include <iostream>

#include <gtest/gtest.h>
#include <util/hook/ExtFuncCallHook.hh>
#include <plthook.h>

using namespace std;
using namespace scaler;

//Reference
extern char __startplt, __endplt, __startpltgot, __endpltgot, __startpltsec, __endpltsec;

TEST(ExtFuncCallHook, locSectionInMem) {
    PMParser parser;
    parser.parsePMMap();

    ExtFuncCallHook *hook = ExtFuncCallHook::getInst();
    hook->locSectionInMem();

    auto &calcPltPtr = hook->fileSecMap.at(hook->fileIDMap.at(parser.curExecFileName)).at(SEC_NAME::PLT);
    auto &calcGotPtr = hook->fileSecMap.at(hook->fileIDMap.at(parser.curExecFileName)).at(SEC_NAME::GOT);
    auto &calcPltSecPtr = hook->fileSecMap.at(hook->fileIDMap.at(parser.curExecFileName)).at(SEC_NAME::PLT_SEC);

    EXPECT_EQ(calcPltPtr.startAddr, &__startplt);
    EXPECT_EQ(calcPltPtr.endAddr, &__endplt);
    EXPECT_EQ(calcGotPtr.startAddr, &__startpltgot);
    EXPECT_EQ(calcGotPtr.endAddr, &__endpltgot);
    EXPECT_EQ(calcPltSecPtr.startAddr, &__startpltsec);
    EXPECT_EQ(calcPltSecPtr.endAddr, &__endpltsec);
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

    hook->locSectionInMem();
    size_t funcId = hook->findExecNameByAddr(addr1);
    EXPECT_EQ(parser.curExecFileName, hook->segAddrFileMap[funcId].fileName);

    void *funcPtr = (void *) printf;
    funcId = hook->findExecNameByAddr(funcPtr);
    auto &execName = hook->segAddrFileMap[funcId].fileName;

    EXPECT_TRUE(execName.find("libc") != std::string::npos);

    hook->segAddrFileMap.clear();
    for (int i = 0; i < 4; i += 2) {
        SegInfo newEntry;
        newEntry.startAddr = (void *) i;
        hook->segAddrFileMap.emplace_back(newEntry);
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

TEST(ExtFuncCallHook, externalNameAndAddress) {

}


int main() {
    nativeFunc();

    ExtFuncCallHook *hook = ExtFuncCallHook::getInst();
    hook->locSectionInMem();

    PMParser pmParser;
    pmParser.parsePMMap();

//Parse current ELF file and see if those method exists and if address matches
    ELFParser parser(pmParser.curExecFileName);
    parser.parse();

    plthook_t *myPltHook;
    if (!myPltHook)
        fprintf(stderr, "Please add the directory containing libNaiveInvocationApp.so to LD_LIBRARY_PATH.\n");

//Find plthook
    plthook_open(&myPltHook, NULL);
//Check plt hook entry size

    unsigned int pos = 0;
    const char *name;
    void **addr;
    int i = 0;

    std::string targetName = "funcEverything";
    for (int i = 0; i < parser.relaFuncName.size(); ++i) {

        plthook_enum(myPltHook, &pos, &name, &addr);
        printf("CurFunc:%s\n", name);
        if (strncmp(name, targetName.c_str(), 14) == 0) {
            printf("funcEverything:%p\n", funcEverything);
//Check if the name is correct
//            EXPECT_EQ(parser.relaFuncName.at(std::string(name)), i);
//
//            //Check if the address is correct
//            auto fileId = hook->fileIDMap.at(pmParser.curExecFileName);
//            void *curGOTAddr = hook->getFuncAddrFromGOTByName(fileId, std::string(name));
//            EXPECT_EQ(curGOTAddr, *addr);

        }
    }

    while (plthook_enum(myPltHook, &pos, &name, &addr) == 0) {
        printf("Left Over:%s\n", name);
    }

}
