#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/ElfParser.h>
#include <util/tool/PMParser.h>
#include <string>
//This is not scaler's plthook
#include <plthook.h>

using namespace std;
using namespace scaler;

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

TEST(ELFParser, parseFuncName) {
    //Invoke libTest, and several system functions
    void *a = malloc(1);
    system("");

    //Parse /proc/self/map to get current executable file name
    PMParser pars;
    pars.parsePMMap();

    //Parse current ELF file and see if those method exists and if address matches
    ELFParser parser(pars.curExecFileName);
    parser.parse();

    plthook_t *myPltHook;
    //Find plthook
    plthook_open(&myPltHook, NULL);
    //Check plt hook entry size

    auto refFuncName = getFuncNameRetByKuboPlthook(myPltHook);

    for (int i = 0; i < parser.relaFuncName.size(); ++i) {
        EXPECT_EQ(parser.relaFuncName[i], refFuncName[i]);
        printf("%s\n", refFuncName[i].c_str());
    }
    printf("------------------------------------------------------------\n");
    for (int i = parser.relaFuncName.size(); i < refFuncName.size(); ++i) {
        printf("%s\n", refFuncName[i].c_str());
    }


}


