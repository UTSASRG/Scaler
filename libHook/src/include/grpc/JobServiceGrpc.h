#include <analyzerserver/JobService.grpc.pb.h>
#include <util/hook/ExtFuncCallHookAsm.hh>
#include <util/datastructure//Vector.h>


#ifndef SCALER_JOBSERVICEGRPC_H
#define SCALER_JOBSERVICEGRPC_H
namespace scaler{
class JobServiceGrpc {
public:
    explicit JobServiceGrpc(std::shared_ptr<grpc::ChannelInterface> channel);

    long createJob();

    bool appendElfImgInfo(ExtFuncCallHookAsm& asmHook);


private:
    std::unique_ptr<scaler::analyzerserv::Job::Stub> stub_;
};
}

#endif //SCALER_JOBSERVICEGRPC_H
