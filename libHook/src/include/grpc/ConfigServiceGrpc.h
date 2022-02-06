#ifndef SCALER_CONFIGSERVICEGRPC_H
#define SCALER_CONFIGSERVICEGRPC_H

namespace scaler{

        class ConfigServiceGrpc {
            public:
            explicit ConfigServiceGrpc(std::shared_ptr <grpc::ChannelInterface> channel);

            // Assembles the client's payload, sends it and presents the response back
            // from the server.
            bool SayHello();

            private:
            std::unique_ptr <scaler::analyzerserv::Info::Stub> stub_;
        };
}
#endif //SCALER_CONFIGSERVICEGRPC_H
