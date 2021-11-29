#include <util/hook/install.h>
#include <util/hook/ExtFuncCallHookBrkpoint.h>
#include <util/tool/StringTool.h>


typedef int (*main_fn_t)(int, char **, char **);

main_fn_t real_main;

std::string execFileName;

int doubletake_main(int argc, char **argv, char **envp) {
    //todo: support different running modes
    DBG_LOG("Installing plthook");
    install([](std::string fileName, std::string funcName) -> bool {
        //todo: User should be able to specify name here. Since they can change filename
        if (funcName == "svcunix_rendezvous_abort") {
            return false;
        } else if (funcName == "svctcp_rendezvous_abort") {
            return false;
        } else if (funcName == "oom") {
            return false;
        } else if (funcName == "longjmp") {
            return false;
        } else if (funcName == "_longjmp") {
            return false;
        } else if (funcName == "siglongjmp") {
            return false;
        } else if (funcName == "__REDIRECT_NTHNL") {
            return false;
        } else if (funcName == "__longjmp_chk") {
            return false;
        } else if (funcName == "jump") {
            return false;
        } else if (funcName == "_exit") {
            return false;
        } else if (funcName == "abort") {
            return false;
        } else if (funcName == "exit") {
            return false;
        } else if (funcName == "quick_exit") {
            return false;
        } else if (funcName == "_Exit") {
            return false;
        } else if (funcName == "fail") {
            return false;
        } else if (funcName == "futex_fatal_error") {
            return false;
        } else if (funcName == "pthread_exit") {
            return false;
        } else if (funcName == "__pthread_unwind_next") {
            return false;
        } else if (funcName == "__pthread_unwind") {
            return false;
        } else if (funcName == "__do_cancel") {
            return false;
        } else if (funcName == "__pthread_exit") {
            return false;
        } else if (funcName == "_dl_fatal_printf") {
            return false;
        } else if (funcName == "_dl_signal_exception") {
            return false;
        } else if (funcName == "_dl_signal_error") {
            return false;
        } else if (funcName == "_dl_reloc_bad_type") {
            return false;
        } else if (funcName == "____longjmp_chk") {
            return false;
        } else if (funcName == "_startup_fatal") {
            return false;
        } else if (funcName == "__ia64_longjmp") {
            return false;
        } else if (funcName == "__longjmp_cancel") {
            return false;
        } else if (funcName == "_setjmp") {
            return false;
        } else if (funcName == "__libc_longjmp") {
            return false;
        } else if (funcName == "__novmxlongjmp") {
            return false;
        } else if (funcName == "__novmx_longjmp") {
            return false;
        } else if (funcName == "__novmxsiglongjmp") {
            return false;
        } else if (funcName == "__novmx__longjmp") {
            return false;
        } else if (funcName == "__novmx__libc_siglongjmp") {
            return false;
        } else if (funcName == "__novmx__libc_longjmp") {
            return false;
        } else if (funcName == "thrd_exit") {
            return false;
        } else if (funcName == "__assert_fail") {
            return false;
        } else if (funcName == "__assert_perror_fail") {
            return false;
        } else if (funcName == "__assert") {
            return false;
        } else if (funcName == "_dl_allocate_tls") {
            return false;
        } else if (funcName == "_dl_allocate_tls_init") {
            return false;
        } else if (funcName == "__call_tls_dtors") {
            return false;
        } else if (funcName == "termination_handler") {
            return false;
        }  else if (funcName == "receive_print_stats") {
            return false;
        } else if (funcName == "nscd_run_prune") {
            return false;
        } else if (funcName == "nscd_run_worker") {
            return false;
        } else if (funcName == "main_loop_poll") {
            return false;
        } else if (funcName == "__chk_fail") {
            return false;
        } else if (funcName == "__longjmp") {
            return false;
        } else if (funcName == "____longjmp_chk") {
            return false;
        } else if (funcName == "__libc_siglongjmp") {
            return false;
        } else if (funcName == "__libc_longjmp") {
            return false;
        } else if (funcName == "__libc_fatal") {
            return false;
        } else if (funcName == "__libc_message") {
            return false;
        } else if (funcName == "__assert_fail_base") {
            return false;
        } else if (funcName == "libc_hidden_proto") {
            return false;
        } else if (funcName == "rtld_hidden_proto") {
            return false;
        } else if (funcName == "err") {
            return false;
        } else if (funcName == "verr") {
            return false;
        } else if (funcName == "errx") {
            return false;
        } else if (funcName == "verrx") {
            return false;
        } else if (funcName == "__REDIRECT") {
            return false;
        } else if (funcName == "_Unwind_RaiseException") {
            return false;
        } else if (funcName == "_ZSt13get_terminatev") {
            return false;
        } else if (funcName == "__cxa_throw") {
            return false;
        } else if (funcName == "__cxa_rethrow") {
            return false;
        } else if (funcName == "__cxa_init_primary_exception") {
            return false;
        } else if (funcName == "__cxa_begin_catch") {
            return false;
        } else if (funcName == "__cxa_bad_cast") {
            return false;
        } else if (funcName == "__cxa_allocate_dependent_exception") {
            return false;
        } else if (funcName == "__cxa_free_exception") {
            return false;
        } else if (funcName == "_Unwind_DeleteException") {
            return false;
        } else if (funcName == "__cxa_current_exception_type") {
            return false;
        } else if (funcName == "__cxa_allocate_exception") {
            return false;
        } else if (funcName == "__cxa_free_dependent_exception") {
            return false;
        } else if (funcName == "_dl_exception_create") {
            return false;
        } else if (funcName == "_dl_catch_exception") {
            return false;
        } else if (funcName == "_dl_catch_error") {
            return false;
        } else if (scaler::strEndsWith(fileName, "libxed.so")) {
            return false;
        } else if (scaler::strEndsWith(fileName, "libxed-ild.so")) {
            return false;
        } else if (scaler::strEndsWith(fileName, "libScalerHook-HookManualAsm.so")) {
            return false;
        } else if (scaler::strEndsWith(fileName, "libScalerHook-HookBrkpoint.so")) {
            return false;
        } else if (scaler::strEndsWith(fileName, "libScalerHook-HookAuto.so")) {
            return false;
        } else {
            //printf("%s\n", fileName.c_str());
            return true;
        }

    }, INSTALL_TYPE::ASM);

//    //Initialization
//    scaler::ExtFuncCallHookAsm *libPltHook = scaler::ExtFuncCallHookAsm::getInst();
//
//    scaler::PmParserC_Linux pmParser;
//    execFileName = pmParser.curExecAbsolutePath;
//
//    //Hook all symbols except certain files
//    //todo:Merged from asmSimpleCase. Only hook current executable for testing
//    libPltHook->install([](std::string fileName, std::string funcName) -> bool {
//        //todo: User should be able to specify name here. Since they can change filename
//        if (fileName == execFileName) {
//            ERR_LOGS("Autoinstall %s:%s", fileName.c_str(), funcName.c_str());
//            return true;
//        } else {
//            return false;
//        }
//    });
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
    DBG_LOG("Uninstalling plthook");
    uninstall(INSTALL_TYPE::ASM);

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
