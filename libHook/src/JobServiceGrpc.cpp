#include<grpc/JobServiceGrpc.h>
#include <util/tool/Logging.h>


using grpc::Channel;
using grpc::ChannelInterface;
using grpc::ClientContext;
using grpc::Status;
using google::protobuf::Empty;
using scaler::analyzerserv::JobInfoMsg;


JobServiceGrpc::JobServiceGrpc(std::shared_ptr<grpc::ChannelInterface> channel) : stub_(
        scaler::analyzerserv::Job::NewStub(channel)) {

}

long JobServiceGrpc::createJob() {
    // Container for the data we expect from the server.
    JobInfoMsg reply;
    Empty emptyParam;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    // The actual RPC.
    Status status = stub_->createJob(&context, emptyParam, &reply);

    // Act upon its status.
    if (status.ok()) {
        INFO_LOGS("New job id created in the analyzer server. ID=%ld", reply.id());
        return reply.id();
    } else {
        ERR_LOGS("Failed to create new job, reason:%s",status.error_message().c_str());
        return -1;
    }
    return 0;
}
