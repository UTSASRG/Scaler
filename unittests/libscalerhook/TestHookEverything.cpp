#include <iostream>
#include <thread>
#include <util/hook/install.h>
#include <FuncWithDiffParms.h>
#include <installTest.h>

using namespace std;


int main() {

    install([](std::string fileName, std::string funcName) -> bool {
        //todo: User should be able to specify name here. Since they can change filename
        if (funcName == "") {
            return false;
        } else if (fileName.length() >= 16 && fileName.substr(fileName.length() - 16, 16) == "libscalerhook.so") {
            return false;
        } else if (funcName.length() >= 26 &&
                   funcName.substr(funcName.length() - 26, 26) != "libscalerhook_installer.so") {
            return false;
        } else {
            return true;
        }
    });

    funcA();
    malloc(190);
    callFuncA();
    printf("Hello\n");
    printf("Hello %d %f\n",1,1.23);


    return 0;
}

