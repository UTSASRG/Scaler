#include <analyzerserver/JobService.grpc.pb.h>


#ifndef SCALER_JOBSERVICEGRPC_H
#define SCALER_JOBSERVICEGRPC_H

class JobServiceGrpc {
public:
    explicit JobServiceGrpc(std::shared_ptr<grpc::ChannelInterface> channel);

    long createJob();


private:
    std::unique_ptr<scaler::analyzerserv::Job::Stub> stub_;
};


#endif //SCALER_JOBSERVICEGRPC_H
