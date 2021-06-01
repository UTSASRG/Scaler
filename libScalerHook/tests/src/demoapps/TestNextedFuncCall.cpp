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

        if (scaler::strEndsWith(fileName, "ld-2.31.so")) {
            return false;
        } else if (scaler::strEndsWith(fileName, "libScalerHook-HookManual.so")) {
            return false;
        } else if (scaler::strEndsWith(fileName, "libstdc++.so.6.0.28")) {
            return false;
        } else if (scaler::strEndsWith(fileName, "libdl-2.31.so")) {
            return false;
        } else {
            //fprintf(stderr, "%s:%s\n", fileName.c_str(), funcName.c_str());
            return true;
        }

    });

    callFuncA();

    return 0;
}