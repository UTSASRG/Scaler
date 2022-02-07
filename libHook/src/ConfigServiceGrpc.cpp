#include <memory>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpc/ConfigServiceGrpc.h>
#include <util/tool/Logging.h>

using scaler::analyzerserv::JobInfoMsg;
using scaler::analyzerserv::YAMLConfigRlt;
using scaler::analyzerserv::ConfigMsg;
using scaler::analyzerserv::BinaryExecResult;
using grpc::Channel;
using grpc::ChannelInterface;
using grpc::ClientContext;
using grpc::Status;
using google::protobuf::Empty;

namespace scaler {
    ConfigServiceGrpc::ConfigServiceGrpc(std::shared_ptr<grpc::ChannelInterface> channel) : stub_(
            scaler::analyzerserv::Config::NewStub(channel)) {
    }

    bool ConfigServiceGrpc::appendYamlConfig(long jobId,std::string configContent) {
        BinaryExecResult reply;
        // Container for the data we expect from the server.
        ConfigMsg configMsg;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;
        configMsg.set_yamlconfig(configContent);
        configMsg.set_jobid(jobId);

        // The actual RPC.
        Status status = stub_->appendYAMLConfig(&context, configMsg, &reply);

        // Act upon its status.
        if (!status.ok()) {
            ERR_LOGS("Failed to send yaml config to analyzer server because: %s", status.error_message().c_str());
            return false;
        } else if (!reply.success()) {
            ERR_LOGS("Failed to send yaml config to analyzer server because: %s", reply.errormsg().c_str());
            return false;
        }
        return true;
    }

    bool ConfigServiceGrpc::getConfigFromServer(long jobId,std::string &configFromServer) {

        YAMLConfigRlt reply;
        // Container for the data we expect from the server.
        JobInfoMsg configMsg;
        configMsg.set_id(jobId);
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;
        // The actual RPC.
        Status status = stub_->getConfigFromServer(&context, configMsg, &reply);

        // Act upon its status.
        if (!status.ok()) {
            ERR_LOGS("Failed to send yaml config to analyzer server because: %s", status.error_message().c_str());
            return false;
        } else if (!reply.success()) {
            ERR_LOGS("Failed to send yaml config to analyzer server because: %s", reply.errormsg().c_str());
            return false;
        }

        INFO_LOG("Config is fetched from server");
        configFromServer = reply.yamlconfig();
        return true;
    }

}