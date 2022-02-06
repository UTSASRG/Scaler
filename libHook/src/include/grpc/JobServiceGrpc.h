#include <analyzerserver/JobService.grpc.pb.h>
#include <util/hook/ExtFuncCallHookAsm.hh>
#include <util/datastructure//Vector.h>


#ifndef SCALER_JOBSERVICEGRPC_H
#define SCALER_JOBSERVICEGRPC_H
namespace scaler {
    class JobServiceGrpc {
    public:
        explicit JobServiceGrpc(std::shared_ptr<grpc::ChannelInterface> channel);

        long createJob();

        bool appendElfImgInfo(ExtFuncCallHookAsm &asmHook);

        bool
        appendTimingMatrix(int64_t processId, int64_t threadId, int64_t timingMatrixRows, int64_t timingMatrixCols,
                           int64_t **timingMatrix,
                           int64_t countingVecRows, int64_t *countingVec);


    private:
        std::unique_ptr<scaler::analyzerserv::Job::Stub> stub_;
    };
}

#endif //SCALER_JOBSERVICEGRPC_H
