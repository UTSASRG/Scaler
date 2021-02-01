#include <util/hook/install.h>
#include <util/hook/ExtFuncCallHook.hh>


typedef int (*main_fn_t)(int, char **, char **);

main_fn_t real_main;

int doubletake_main(int argc, char **argv, char **envp) {
    //Initialization
    scaler::ExtFuncCallHook_Linux *libPltHook = scaler::ExtFuncCallHook_Linux::getInst();

    //Hook all symbols except certain files

//    libPltHook->install([](std::string fileName, std::string funcName) -> bool {
//        //todo: User should be able to specify name here. Since they can change filename
//        if (fileName.substr(funcName.length()>=16 && fileName.length() - 16, 16) == "libscalerhook.so") {
//            return false;
//        } else if (funcName.length()>=26 && funcName.substr(funcName.length() - 26, 26) != "libscalerhook_installer.so"){
//            return false;
//        } else {
//            return true;
//        }
//    });

    //todo: Here
    libPltHook->install([](std::string fileName, std::string funcName) -> bool {
        //todo: User should be able to specify name here. Since they can change filename
        if (funcName == "funcA" || funcName == "funcEverything") {
            return true;
        }else{
            return false;
        }
    });

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
