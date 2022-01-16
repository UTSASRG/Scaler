#include <analyzerserver/InfoService.grpc.pb.h>


    class InfoServiceGrpc {
    public:
        explicit InfoServiceGrpc(std::shared_ptr<grpc::ChannelInterface> channel);

        // Assembles the client's payload, sends it and presents the response back
        // from the server.
        void SayHello();

    private:
        std::unique_ptr<scaler::analyzerserv::Info::Stub> stub_;
    };


