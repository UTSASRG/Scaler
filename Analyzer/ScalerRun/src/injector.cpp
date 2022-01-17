


#include <cstdlib>
#include <sstream>
#include <grpc/InfoServiceGrpc.h>
#include <util/tool/Logging.h>
#include <grpcpp/create_channel.h>
#include <iostream>
#include <getopt.h>
#include <yaml-cpp/eventhandler.h>
#include <yaml-cpp/yaml.h>

using namespace std;

std::string configFilePath = "";
YAML::Node config;

void printUsage(void) {
    fprintf(stderr,
            "ScalerRun [option]... -c <config.yml> <target_program> [target_program_options] ...\n"
            "  -t|--test                  Perform some quick tests on e environment before scaler runs\n"
            "  -v|--version            Pring ScalerRun Version\n"
    );
}

void printVersion() {
    INFO_LOGS("ScalerRun Ver %s", CMAKE_SCALERRUN_VERSION);
}

bool testConnection() {
    std::string grpcAddr = config["hook"]["analyzerserv"]["address"].as<std::string>("null").c_str();
    INFO_LOGS("Testing connection to server at %s", grpcAddr.c_str());
    //Find server address and port from config file
    InfoServiceGrpc infoService(grpc::CreateChannel(grpcAddr, grpc::InsecureChannelCredentials()));
    return infoService.SayHello();
}

int runScaler(int argc, char **argv) {
    //Say Hello to server
    if (!testConnection()) {
        ERR_LOG("Failed to connect to analyserver. Analyzing capability is not available.");
        ERR_LOG("Please check internet connection. If you think this is a bug, please inform the developer.");

        while (true) {
            std::cout << "Do you want to continue to run Scaler? [y/n]: ";
            char userResponse = 0;
            std::cin >> userResponse;
            if (userResponse == 'n') {
                exit(-1);
            } else if (userResponse == 'y') {
                break;
            }
        }
    }
    DBG_LOGS("Passing config path %s to libHook in environment variable SCALER_HOOK_CONFIG_FILE",
             configFilePath.c_str());
    setenv("SCALER_HOOK_CONFIG_FILE", configFilePath.c_str(), true);
    std::string scalerBinPath = config["hook"]["scalerhome"].as<std::string>("null").c_str();
    if(scalerBinPath=="null"){
        ERR_LOG("You must specify scaler path");
        exit(-1);
    }
    scalerBinPath+="/libScalerHook-HookAuto.so";
    DBG_LOGS("Scaler binary is at %s according to the configuration",scalerBinPath.c_str());
    std::stringstream ss;
    std::string scalerBin(argv[1]);
    ss << "LD_PRELOAD=" << scalerBinPath << " ";
    for (int i = 0; i < argc; ++i) {
        ss << argv[i] << " ";
    }
    DBG_LOGS("%s\n", ss.str().c_str());
    return system(ss.str().c_str());
}

int main(int argc, char **argv) {
    INFO_LOGS("ScalerRun Ver %s", CMAKE_SCALERRUN_VERSION);
    //Parse Commandline arguments

    if (argc == 1) {
        printUsage();
        exit(0);
    }
    static const struct option long_options[] =
            {
                    {"config",  required_argument, NULL, 'c'},
                    {"test",    no_argument,       NULL, 't'},
                    {"version", no_argument,       NULL, 'v'},
                    {NULL, 0,                      NULL, 0}
            };

    int opt = 0;
    int options_index = 0;
    bool testConnectionOnly = false;
    bool hasConfig = false;
    bool cont=true;

    int targetProgramArgIndex=0;
    for(int i=0;i<argc;++i){
        if(std::string(argv[i])=="--config" || std::string(argv[i])=="-c"){
            targetProgramArgIndex=i+2;
            break;
        }
    }

    while ((opt = getopt_long(targetProgramArgIndex, argv, "tvc:", long_options, &options_index)) != EOF && cont) {
        switch (opt) {
            case 0 :
                break;
            case 'v' :
                printVersion();
                return 0;
                break;
            case 't' :
                testConnectionOnly = true;
                break;
            case 'c' :
                try {
                    config = YAML::LoadFile(optarg);
                    configFilePath = optarg;
                } catch (YAML::Exception &e) {
                    ERR_LOGS("Cannot parse %s ErrMsg: %s", optarg, e.what());
                    exit(-1);
                } catch (std::ios_base::failure &e) {
                    ERR_LOGS("Cannot parse %s ErrMsg: %s", optarg, e.what());
                    exit(-1);
                }
                cont=false;
                break;
        }
    }

    if (testConnectionOnly) {
        if (configFilePath == "") {
            ERR_LOG("You need to specify -c in order to perform tests");
        }
        testConnection();
        return 0;
    }

    if (configFilePath == "") {
        ERR_LOG("You need to specify -c in order to run scaler");
    }
    if(optind==argc){
        ERR_LOG("You must specify program and arguments to start profiling");
        exit(-1);
    }
    return runScaler(argc-targetProgramArgIndex, argv+targetProgramArgIndex);
}