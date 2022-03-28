#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/ElfParser.h>
#include <util/tool/ProcInfoParser.h>
#include <string>
//This is not scaler's plthook
#include <plthook.h>

using namespace std;
using namespace scaler;

//todo; Put old test code to tests folder

vector<string> getFuncNameRetByKuboPlthook(plthook_t *plthook) {
    unsigned int pos = 0;
    const char *name;
    void **addr;
    int i;
    vector<string> results;
    while (plthook_enum(plthook, &pos, &name, &addr) == 0) {
        //printf("   %s\n", name);
        results.emplace_back(std::string(name));
    }
    return results;
}

TEST(ELFParser_Linux, parseFuncName) {
    //Invoke libTest, and several system functions
    void *a = malloc(1);

    PmParser pmParser;

    //Parse current ELF file and see if those method exists and if address matches
//    ELFParser parser(pmParser.curExecAbsolutePath);
//
//    plthook_t *myPltHook;
//    //Find plthook
//    plthook_open(&myPltHook,NULL);
//
//    if (!myPltHook)
//        fprintf(stderr, "Please add the directory containing libNaiveInvocationApp.so to LD_LIBRARY_PATH.\n");
//
//    auto refFuncName = getFuncNameRetByKuboPlthook(myPltHook);
//
//
////    for (int i = 0; i < parser.relaFuncName.size(); ++i) {
////        EXPECT_EQ(parser.relaFuncName.at(refFuncName[i]), i);
////    }
//
//    // There might be entries discovered by plthook. However, they are external symbols rather than functions, meaning
//    // they don't exist in PLT, We don't need to consider them.
//    //for (int i = parser.relaFuncName.size(); i < refFuncName.size(); ++i) {
//    //    printf("%s\n", refFuncName[i].c_str());
}




