#include <util/hook/install.h>
#include <util/hook/ExtFuncCallHookAsm.hh>


typedef int (*main_fn_t)(int, char **, char **);

main_fn_t real_main;

std::string execFileName;

int doubletake_main(int argc, char **argv, char **envp) {
    //Initialization
    scaler::ExtFuncCallHookAsm *libPltHook = scaler::ExtFuncCallHookAsm::getInst();

    scaler::PmParserC_Linux pmParser;
    execFileName = pmParser.curExecAbsolutePath;

    //Hook all symbols except certain files
    //todo:Merged from asmSimpleCase. Only hook current executable for testing
    libPltHook->install([](std::string fileName, std::string funcName) -> bool {
        //todo: User should be able to specify name here. Since they can change filename
        if (fileName == execFileName) {
            ERR_LOGS("Autoinstall %s:%s", fileName.c_str(), funcName.c_str());
            return true;
        } else {
            return false;
        }
    });

    libPltHook->saveAllSymbolId();
    libPltHook->saveCommonFuncID();

//    libPltHook->install([](std::string fileName, std::string funcName) -> bool {
//        //todo: User should be able to specify name here. Since they can change filename
//        if (funcName == "") {
//            return false;
//        } else if (fileName.length() >= 16 && fileName.substr(fileName.length() - 16, 16) == "libscalerhook.so") {
//            return false;
//        } else if (funcName.length() >= 26 &&
//                   funcName.substr(funcName.length() - 26, 26) != "libscalerhook_installer.so") {
//            return false;
//        }  else {
//            return true;
//        }
    //todo: User should be able to specify name here. Since they can change filename
//        if (funcName == "") {
//            return false;
//        } else if (fileName.length() >= 16 && fileName.substr(fileName.length() - 16, 16) == "libscalerhook.so") {
//            return false;
//        } else if (funcName.length() >= 26 &&
//                   funcName.substr(funcName.length() - 26, 26) != "libscalerhook_installer.so") {
//            return false;
//        }  else {
//            return true;
//        }
//    });

    // Call the program's main function
    int ret = real_main(argc, argv, envp);

    return ret;
}

extern "C" int __libc_start_main(main_fn_t, int, char **, void (*)(), void (*)(), void (*)(),
                                 void *) __attribute__((weak, alias("doubletake_libc_start_main")));

extern "C" int doubletake_libc_start_main(main_fn_t main_fn, int argc, char **argv, void (*init)(), void (*fini)(),
                                          void (*rtld_fini)(), void *stack_end) {
    // Find the real __libc_start_main
    auto real_libc_start_main = (decltype(__libc_start_main) *) dlsym(RTLD_NEXT, "__libc_start_main");
    // Save the program's real main function
    real_main = main_fn;
    // Run the real __libc_start_main, but pass in doubletake's main function
    return real_libc_start_main(doubletake_main, argc, argv, init, fini, rtld_fini, stack_end);
}
