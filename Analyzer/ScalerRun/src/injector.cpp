


#include <cstdlib>
#include <sstream>
#include <HookConfig.h>
#include <grpc/InfoServiceGrpc.h>
#include <util/tool/Logging.h>
#include <grpcpp/create_channel.h>
#include <iostream>
#include <getopt.h>
#include <yaml-cpp/eventhandler.h>
#include <yaml-cpp/yaml.h>

using namespace std;

YAML::Node config;

void printUsage(void) {
    fprintf(stderr,
            "ScalerRun -c <config.yml> [option]... <target_program> [target_program_options] ...\n"
            "  -t|--test                  Test connection to the analyzer server without profilingr\n"
            "  -v|--version            Pring ScalerRun Version\n"
    );
}

void printVersion() {
    printf("ScalerRun Ver %s", CMAKE_SCALERRUN_VERSION);
}

void testConnection() {
    InfoServiceGrpc infoService(grpc::CreateChannel("localhost:3060", grpc::InsecureChannelCredentials()));
    infoService.SayHello();
}

int runScaler(int argc, char **argv) {
    //Say Hello to server
    InfoServiceGrpc infoService(grpc::CreateChannel("localhost:3060", grpc::InsecureChannelCredentials()));
    if (!infoService.SayHello()) {
        std::cerr << "Failed to connect to analyserver. Analyzing capability is not available." << std::endl;
        std::cerr << "Please check internet connection. If you think this is a bug, please inform the developer."
                  << std::endl;

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

    INFO_LOGS("Reading config files Ver %s", CMAKE_SCALERRUN_VERSION);

    std::stringstream ss;
    std::string scalerBin(argv[1]);
    ss << "LD_PRELOAD=" << scalerBin << " ";
    for (int i = 2; i < argc; ++i) {
        ss << argv[i] << " ";
    }
    printf("%s\n", ss.str().c_str());
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
    while ((opt = getopt_long(argc, argv, "tvc:", long_options, &options_index)) != EOF) {
        switch (opt) {
            case 0 :
                break;
            case 'v' :
                printVersion();
                return 0;
                break;
            case 't' :
                testConnection();
                return 0;
                break;
            case 'c' :
                try {
                    config = YAML::LoadFile(optarg);
                } catch (YAML::Exception &e) {
                    ERR_LOGS("Cannot parse %s ErrMsg: %s", optarg, e.what());
                    exit(-1);
                }catch (std::ios_base::failure &e) {
                    ERR_LOGS("Cannot parse %s ErrMsg: %s", optarg, e.what());
                    exit(-1);
                }
                break;
        }
    }

    return runScaler(argc, argv);
}