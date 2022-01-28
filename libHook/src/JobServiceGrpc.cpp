#include<grpc/JobServiceGrpc.h>
#include <util/tool/Logging.h>
#include <util/config/Config.h>


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


using namespace scaler;

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
        ERR_LOGS("Failed to create new job because: %s", status.error_message().c_str());
        return -1;
    }
    return 0;
}

bool JobServiceGrpc::appendElfImgInfo(ExtFuncCallHookAsm &asmHook) {
    BinaryExecResult reply;
    ELFImgInfoMsg elfInfoParm;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;


    const auto &clientWriter = stub_->appendElfImgInfo(&context, &reply);

    //Note: assume sequential
    for (int fileID = 0; fileID < asmHook.elfImgInfoMap.getSize(); ++fileID) {
        const auto &elfImgInfo = asmHook.elfImgInfoMap[fileID];
        ELFImgInfoMsg elfInfoMsg;
        elfInfoMsg.set_scalerid(fileID);
        elfInfoMsg.set_valid(elfImgInfo.elfImgValid);
        if (elfImgInfo.elfImgValid) {
            elfInfoMsg.set_filepath(elfImgInfo.filePath);
            elfInfoMsg.set_addrstart(reinterpret_cast<long>(elfImgInfo.baseAddrStart));
            elfInfoMsg.set_addrend(reinterpret_cast<long>(elfImgInfo.baseAddrEnd));
            elfInfoMsg.set_pltstartaddr(reinterpret_cast<long>(elfImgInfo.pltStartAddr));
            elfInfoMsg.set_pltsecstartaddr(reinterpret_cast<long>(elfImgInfo.pltSecStartAddr));
            elfInfoMsg.set_jobid(Config::curJobId);
            DBG_LOGS("Sending %d/%zd", fileID + 1, asmHook.elfImgInfoMap.getSize());
            for (int symbolId:elfImgInfo.scalerIdMap) {
                const auto &symbolInfo = asmHook.allExtSymbol[symbolId];
                ELFSymbolInfoMsg &elfSymbolInfoMsg = *elfInfoMsg.add_symbolinfointhisfile();
                elfSymbolInfoMsg.set_scalerid(symbolInfo.scalerSymbolId);
                elfSymbolInfoMsg.set_symbolname(symbolInfo.symbolName);
                std::string symbolType = "";
                switch (symbolInfo.type) {
                    case STT_NOTYPE:
                        symbolType = "STT_NOTYPE";
                        break;
                    case STT_OBJECT:
                        symbolType = "STT_OBJECT";
                        break;
                    case STT_FUNC:
                        symbolType = "STT_FUNC";
                        break;
                    case STT_SECTION:
                        symbolType = "STT_SECTION";
                        break;
                    case STT_FILE:
                        symbolType = "STT_FILE";
                        break;
                    case STT_COMMON:
                        symbolType = "STT_COMMON";
                        break;
                    case STT_TLS:
                        symbolType = "STT_TLS";
                        break;
                    case STT_NUM:
                        symbolType = "STT_NUM";
                        break;
                    default:
                        symbolType = "UNKNOWN(" + std::to_string(symbolInfo.type) + ")";
                        break;
                }
                elfSymbolInfoMsg.set_symboltype(symbolType);
                std::string bindType = "";

                switch (symbolInfo.bind) {
                    case STB_LOCAL:
                        bindType = "STB_LOCAL";
                        break;
                    case STB_GLOBAL:
                        bindType = "STB_GLOBAL";
                        break;
                    case STB_WEAK:
                        bindType = "STB_WEAK";
                        break;
                    case STB_NUM:
                        bindType = "STB_NUM";
                        break;
                    default:
                        bindType = "UNKNOWN(" + std::to_string(symbolInfo.type) + ")";
                        break;
                }
                elfSymbolInfoMsg.set_bindtype(bindType);
                elfSymbolInfoMsg.set_libfileid(symbolInfo.libraryFileScalerID);
                elfSymbolInfoMsg.set_gotaddr(reinterpret_cast<long>(symbolInfo.gotEntry));
                elfSymbolInfoMsg.set_hooked(symbolInfo.isHooked());
                elfSymbolInfoMsg.set_hookedid(symbolInfo.hookedId);
            }
        }
        // The actual RPC.
        clientWriter->Write(elfInfoMsg);
    }
    INFO_LOG("Waiting for server processing of elfimgInfo to complete");
    clientWriter->WritesDone();
    const auto &status = clientWriter->Finish();

    if (!status.ok()) {
        ERR_LOGS("Failed to send elfinfo to analyzer server because: %s", status.error_message().c_str());
        return false;
    } else if (!reply.success()) {
        ERR_LOGS("Failed to send elfinfo info to analyzer server because: %s", reply.errormsg().c_str());
        return false;
    }

    return true;
}

pthread_mutex_t timingLock=PTHREAD_MUTEX_INITIALIZER;
bool JobServiceGrpc::appendTimingMatrix(int64_t threadId, int64_t timingMatrixRows, int64_t timingMatrixCols,
                                        int64_t **timingMatrix,
                                        int64_t countingVecRows, int64_t *countingVec) {
    BinaryExecResult reply;
    TimingMsg timingMsg;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    timingMsg.set_jobid(Config::curJobId);
    timingMsg.set_threadid(threadId);
    timingMsg.set_timgmatrixrows(timingMatrixRows);
    timingMsg.set_timgmatrixcols(timingMatrixCols);
    for (int i = 0; i < timingMatrixRows; ++i) {
        for (int j = 0; j < timingMatrixCols; ++j) {
            timingMsg.add_timgmatrixval(timingMatrix[i][j]);
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
