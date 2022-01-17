#include <util/tool/StringTool.h>
#include <util/hook/install.h>
#include <util/tool/Logging.h>
#include <dlfcn.h>
#include <util/tool/FileTool.h>
#include <util/config/Config.h>
#include <exceptions/ScalerException.h>
#include <grpcpp/create_channel.h>
#include <grpc/JobServiceGrpc.h>
#include <grpc/ChannelPool.h>

typedef int (*main_fn_t)(int, char **, char **);

main_fn_t real_main;

std::string execFileName;

int doubletake_main(int argc, char **argv, char **envp) {
    INFO_LOGS("libHook Ver %s", CMAKE_SCALERRUN_VERSION);

    char *SCALER_HOOK_CONFIG_FILE = getenv("SCALER_HOOK_CONFIG_FILE");

    if (!SCALER_HOOK_CONFIG_FILE || std::string(SCALER_HOOK_CONFIG_FILE).empty()) {
        ERR_LOG("Cannot find SCALER_HOOK_CONFIG_FILE, please consider using scaler run or manually set this environment variable to the absolute path of the config file.");
        exit(-1);
    }

    DBG_LOG("Parsing config file.");
    try {
        scaler::Config::globalConf = YAML::LoadFile(SCALER_HOOK_CONFIG_FILE);
        DBG_LOGS("%s", scaler::Config::globalConf["hook"]["gccpath"].as<std::string>("null").c_str());
    } catch (YAML::Exception &e) {
        ERR_LOGS("Cannot parse %s ErrMsg: %s", SCALER_HOOK_CONFIG_FILE, e.what());
        exit(-1);
    } catch (std::ios_base::failure &e) {
        ERR_LOGS("Cannot parse %s ErrMsg: %s", SCALER_HOOK_CONFIG_FILE, e.what());
        exit(-1);
    }

    DBG_LOG("Requesting a new jobid");
    std::string grpcAddr = scaler::Config::globalConf["hook"]["analyzerserv"]["address"].as<std::string>(
            "null").c_str();
    bool isSecure = !scaler::Config::globalConf["hook"]["analyzerserv"]["insecure"].as<bool>(false);
    if (isSecure) {
        fatalError("grpc secure mode currently not implemented, please temporarily set grps.insecure to true");
        return -1;
    }


    //todo: support different running modes
    DBG_LOG("Installing plthook");
    install([](std::string fileName, std::string funcName) -> bool {
        //todo: User should be able to specify name here. Since they can change filename
        if (funcName == "__sigsetjmp") {
            return false;
        } else if (funcName == "__tunable_get_val") {
            return false;
        } else if (funcName == "__tls_get_addr") {
            return false;
        } else if (funcName == "_dl_sym") {
            return false;
        } else if (funcName == "_dl_find_dso_for_object") {
            return false;
        } else if (funcName == "svcunix_rendezvous_abort") {
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
        } else if (funcName == "receive_print_stats") {
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


    INFO_LOGS("Creating a new job in analyzer server at %s ......", grpcAddr.c_str());
    scaler::ChannelPool::channel = grpc::CreateChannel(grpcAddr, grpc::InsecureChannelCredentials());

    //Find server address and port from config file
    JobServiceGrpc jobService(scaler::ChannelPool::channel);
    scaler::Config::curJobId = jobService.createJob();

    if(scaler::Config::curJobId==-1){
        //todo: Handle offline profiling mode
        ERR_LOG("Cannot create job. Data will not be sent to analyzer server.");
    }

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
    using namespace scaler;
    // Find the real __libc_start_main
    auto real_libc_start_main = (decltype(__libc_start_main) *) dlsym(RTLD_NEXT, "__libc_start_main");
    if (!real_libc_start_main) {
        fatalError("Cannot find __libc_start_main.");
        return -1;
    }
    // Save the program's real main function
    real_main = main_fn;
    // Run the real __libc_start_main, but pass in doubletake's main function
    return real_libc_start_main(doubletake_main, argc, argv, init, fini, rtld_fini, stack_end);
}
