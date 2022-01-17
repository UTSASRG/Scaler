#include<grpc/InfoServiceGrpc.h>
#include <memory>
#include <util/tool/Logging.h>

using scaler::analyzerserv::HelloMsg;
using grpc::Channel;
using grpc::ChannelInterface;
using grpc::ClientContext;
using grpc::Status;
using google::protobuf::Empty;

InfoServiceGrpc::InfoServiceGrpc(std::shared_ptr<ChannelInterface> channel) : stub_(
        scaler::analyzerserv::Info::NewStub(channel)) {


}

bool InfoServiceGrpc::SayHello() {
    // Container for the data we expect from the server.
    HelloMsg reply;
    Empty emptyParam;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    // The actual RPC.
    Status status = stub_->sayHello(&context, emptyParam, &reply);

    // Act upon its status.
    if (status.ok()) {
        INFO_LOG("Connection successful");
        INFO_LOGS("-> api:%s", reply.api().c_str());
        INFO_LOGS("-> appName:%s", reply.appname().c_str());
        INFO_LOGS("-> version:%s", reply.appversion().c_str());
        INFO_LOGS("-> slogan:%s", reply.slogan().c_str());
        return true;
    } else {
        ERR_LOG("Cannot connect to Analyzer Server.");
        return false;
    }

}


