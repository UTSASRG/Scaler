#include<gprc/InfoServiceGrpc.h>
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

void InfoServiceGrpc::SayHello() {
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
        ERR_LOGS("Connection successful\napi:%s\nappName:%s\nversion=%s\nslogan:%s\n", reply.api().c_str(),
                 reply.appname().c_str(), reply.appversion().c_str(), reply.slogan().c_str());
    } else {
        ERR_LOG("Cannot connect to Analyzer Server failed.");
    }

}


