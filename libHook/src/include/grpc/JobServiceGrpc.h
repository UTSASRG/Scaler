#ifndef SCALER_JOBSERVICEGRPC_H
#define SCALER_JOBSERVICEGRPC_H

#include <analyzerserver/JobService.grpc.pb.h>
#include <util/hook/ExtFuncCallHookAsm.hh>
#include <util/datastructure//Vector.h>
#include <util/config/Config.h>
#include <util/datastructure/Matrix.h>

namespace scaler {
    class JobServiceGrpc {
    public:
        explicit JobServiceGrpc(std::shared_ptr<grpc::ChannelInterface> channel);

        long createJob();

        bool appendElfImgInfo(ExtFuncCallHookAsm &asmHook);

        template<typename ValType>
        bool appendTimingMatrix(int64_t processId, int64_t threadId, Matrix<ValType> &timingMatrix,
                                int64_t countingVecRows, int64_t *countingVec) {
            using grpc::Channel;
            using grpc::ChannelInterface;
            using grpc::ClientContext;
            using grpc::Status;
            using google::protobuf::Empty;
            using scaler::analyzerserv::JobInfoMsg;
            using scaler::analyzerserv::BinaryExecResult;
            using scaler::analyzerserv::ELFImgInfoMsg;
            using scaler::analyzerserv::ELFSymbolInfoMsg;
            using scaler::analyzerserv::TimingMsg;


            BinaryExecResult reply;
            TimingMsg timingMsg;
            // Context for the client. It could be used to convey extra information to
            // the server and/or tweak certain RPC behaviors.
            timingMsg.set_jobid(Config::curJobId);
            timingMsg.set_processid(processId);
            timingMsg.set_threadid(threadId);
            timingMsg.set_timgmatrixrows(timingMatrix.getRows());
            timingMsg.set_timgmatrixcols(timingMatrix.getCols());
            for (int i = 0; i < timingMatrix.getRows(); ++i) {
                for (int j = 0; j < timingMatrix.getCols(); ++j) {
                    timingMsg.add_timgmatrixval(timingMatrix.get(i, j));
                }
            }

            timingMsg.set_countingvecrows(countingVecRows);
            for (int i = 0; i < countingVecRows; ++i) {
                timingMsg.add_countingvecval(countingVec[i]);
            }


            ClientContext context;
            const auto &status = stub_->appendTimingMatrix(&context, timingMsg, &reply);
            INFO_LOG("Waiting for server processing of timingMatrix to complete");

            if (!status.ok()) {
                ERR_LOGS("Failed to send timing info to analyzer server for thread %lu because: %s", pthread_self(),
                         status.error_message().c_str());
                return false;
            } else if (!reply.success()) {
                ERR_LOGS("Failed to send timing info to analyzer server for thread %lu because: %s", pthread_self(),
                         reply.errormsg().c_str());
                return false;
            }

            return true;
        }


    private:
        std::unique_ptr<scaler::analyzerserv::Job::Stub> stub_;
    };
}

#endif //SCALER_JOBSERVICEGRPC_H
