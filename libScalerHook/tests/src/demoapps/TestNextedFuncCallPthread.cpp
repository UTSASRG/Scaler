#include <CallFunctionCall.h>
#include <util/hook/install.h>
#include <util/tool/StringTool.h>
#include <signal.h>

using namespace std;

volatile int DEBUGGER_WAIT = 1;

void test_continue() {
    DEBUGGER_WAIT = 0;
}

int main() {
//    while (DEBUGGER_WAIT) {
        //Let gdb break
//    }


    install([](std::string fileName, std::string funcName) -> bool {
        //todo: User should be able to specify name here. Since they can change filename

        if (scaler::strContains(fileName, "/ld-")) {
            return false;
        } else if (scaler::strContains(fileName, "/liblibScalerHook-HookManual")) {
            return false;
        } else if (scaler::strContains(fileName, "/libstdc++")) {
            return false;
        } else if (scaler::strContains(fileName, "/libdl-")) {
            return false;
        } else {
            fprintf(stderr, "%s:%s\n", fileName.c_str(), funcName.c_str());
            return true;
        }

    });

    callFuncA();

    return 0;
}