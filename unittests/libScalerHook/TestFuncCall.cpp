
#include <cstdio>
#include <FuncWithDiffParms.h>
#include <TenThousandFunc.h>
//#include <plthook.h>
#include <util/hook/ExtFuncCallHook.hh>

using namespace std;
using namespace scaler;

//void test_plthook_enum(plthook_t *plthook) {
//    unsigned int pos = 0;
//    const char *name;
//    void **addr;
//    int i;
//
//    while (plthook_enum(plthook, &pos, &name, &addr) == 0) {
//        printf("   %s\n", name);
//    }
//
//}

#include <FuncWithDiffParms.h>
#include <TenThousandFunc.h>
#include <cassert>


extern int EXTVAR_VAR1;

static int EXTVAR_VAR2;

extern int SELF_EXTERN;
int SELF_EXTERN = 0;

int main() {
    findBaseAddrInALibrary();

//    printf("Calling funcA\n");
//    funcA();
//    printf("Calling funcB\n");
//    funcB(1);
//    printf("Calling funcC\n");
//    funcC(1, 2);
//    printf("Calling funcD\n");
//    funcD(1, 2, 3);
//    printf("Calling funcE\n");
//    funcE(1, 2, 3);
//    structparm strP;
//
//    structparm s;
//    int e, f, g, h, i, j, k;
//    long double ld;
//    double m, n;
//    __m256 y;
//    __m512 z;
//    funcEverything(e, f, s, g, h, ld, m, y, z, n, i, j, k);
//
//    auto a1 = EXTVAR_VAR1 + 1;
//    a1 += A::asdf;
//    a1 += EXTVAR_VAR2;
//    a1 += SELF_EXTERN;
//
//    PmParser_Linux pmParser;
//
//    ExtFuncCallHook *hook = ExtFuncCallHook::getInst();
//    hook->locateRequiredSecAndSeg();
//
//    auto &curFileSecInfo = hook->fileSecInfoMap.at(hook->fileIDMap.at(pmParser.curExecFileName));
//    auto &calcGotPtr = curFileSecInfo.at(SEC_NAME::GOT);
//
//
////Parse current ELF file and see if those method exists and if address matches
//    ELFParser elfParser(pmParser.curExecFileName);
//
//    plthook_t *myPltHook;
//    if (!myPltHook)
//        fprintf(stderr, "Please add the directory containing libNaiveInvocationApp.so to LD_LIBRARY_PATH.\n");
//
////Find plthook
//    plthook_open(&myPltHook, NULL);
////Check plt hook entry size
//
//    unsigned int pos = 0;
//    const char *name;
//    void **addr;
//
//    printf("elfParser.relaFuncName.size():%lu\n", elfParser.relaFuncName.size());
//
//
//    std::string targetName = "funcA";
//    for (int i = 0; i < elfParser.relaFuncName.size(); ++i) {
//        plthook_enum(myPltHook, &pos, &name, &addr);
//        printf("CurFunc:%s\n", name);
//        if (strncmp(name, targetName.c_str(), 5) == 0) {
//            void *funcAddr = getFuncAddr(name);
//
//            assert(funcAddr == *addr);
////Check if the name is correct
//            auto fileId = hook->fileIDMap.at(pmParser.curExecFileName);
//            void *curGOTAddr = nullptr;//hook->getFuncAddrFromGOTByName(fileId, std::string(name));
//            assert(curGOTAddr == *addr);
//
//        }
//    }
//
//    while (plthook_enum(myPltHook, &pos, &name, &addr) == 0) {
//        printf("Left Over:%s\n", name);
//    }
//
//
//
//
//    return 0;
}