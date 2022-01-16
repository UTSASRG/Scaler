


#include <cstdlib>
#include <sstream>
#include <HookConfig.h>
#include <grpc/InfoServiceGrpc.h>
#include <util/tool/Logging.h>
#include <grpcpp/create_channel.h>
#include <iostream>

using namespace std;

int main(int argc, char **argv) {
    INFO_LOGS("ScalerRun Ver %s", CMAKE_SCALERRUN_VERSION);
    InfoServiceGrpc infoService(grpc::CreateChannel("localhost:3060", grpc::InsecureChannelCredentials()));
    if (!infoService.SayHello()) {
        std::cerr << "Failed to connect to analyserver. Analyzing capability is not available." << std::endl;
        std::cerr << "Please check internet connection. If you think this is a bug, please inform the developer.";

        while (true) {
            std::cout << "Do you want to continue to run Scaler? [y/n]";
            char userResponse = 0;
            std::cin >> userResponse;
            if (userResponse == 'n') {
                exit(-1);
            } else {
                break;
            }
        }

    }

    return 0;

    std::stringstream ss;
    std::string scalerBin(argv[1]);
    ss << "LD_PRELOAD=" << scalerBin << " ";
    for (int i = 2; i < argc; ++i) {
        ss << argv[i] << " ";
    }
    printf("%s\n", ss.str().c_str());
    return system(ss.str().c_str());
    return 0;
}