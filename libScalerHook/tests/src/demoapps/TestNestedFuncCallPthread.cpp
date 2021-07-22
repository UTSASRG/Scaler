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


//    install([](std::string fileName, std::string funcName) -> bool {
//        //todo: User should be able to specify name here. Since they can change filename
//
//        if (fileName=="/home/st/Projects/Scaler/cmake-build-debug/libScalerHook/tests/libScalerHook-demoapps-FuncNestedCall") {
//            return true;
//        } else {
//            return false;
//        }
//
//    });

    callFuncA();

    return 0;
}