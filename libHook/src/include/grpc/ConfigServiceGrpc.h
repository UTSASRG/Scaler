#ifndef SCALER_CONFIGSERVICEGRPC_H
#define SCALER_CONFIGSERVICEGRPC_H


#include <analyzerserver/ConfigService.grpc.pb.h>

namespace scaler {

    class ConfigServiceGrpc {
    public:
        explicit ConfigServiceGrpc(std::shared_ptr<grpc::ChannelInterface> channel);

        // Assembles the client's payload, sends it and presents the response back
        // from the server.
        bool appendYamlConfig(long jobId,std::string configContent);


        bool getConfigFromServer(long jobId, std::string &configFromServer);
    private:
        std::unique_ptr<scaler::analyzerserv::Config::Stub> stub_;


    };
}
#endif //SCALER_CONFIGSERVICEGRPC_H
