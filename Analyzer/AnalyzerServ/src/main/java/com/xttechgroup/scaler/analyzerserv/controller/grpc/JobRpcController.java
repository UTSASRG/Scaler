package com.xttechgroup.scaler.analyzerserv.controller.grpc;

import com.google.protobuf.Empty;
import com.xttechgroup.scaler.analyzerserv.*;
import com.xttechgroup.scaler.analyzerserv.models.Job.ELFSymbolEntity;
import com.xttechgroup.scaler.analyzerserv.models.Job.ElfImgInfoEntity;
import com.xttechgroup.scaler.analyzerserv.models.Job.JobEntity;
import com.xttechgroup.scaler.analyzerserv.models.Job.JobRepo;
import io.grpc.Metadata;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;
import io.grpc.stub.StreamObserver;
import net.devh.boot.grpc.server.service.GrpcService;
import org.springframework.beans.factory.annotation.Autowired;

import java.util.HashSet;
import java.util.Optional;
import java.util.Set;

@GrpcService
public class JobRpcController extends JobGrpc.JobImplBase {
    @Autowired
    JobRepo jobRepository;

    @Override
    public void createJob(Empty request, StreamObserver<JobInfoMsg> responseObserver) {
        JobEntity newJob = jobRepository.save(new JobEntity());
        JobInfoMsg reply = JobInfoMsg.newBuilder()
                .setId(newJob.id)
                .build();
        responseObserver.onNext(reply);
        responseObserver.onCompleted();
    }

    @Override
    public StreamObserver<ELFImgInfoMsg> appendElfImgInfo(StreamObserver<BinaryExecResult> responseObserver) {

        return new StreamObserver<ELFImgInfoMsg>() {
            Long jobid = null;
            Set<ElfImgInfoEntity> elfImgInfos=new HashSet<ElfImgInfoEntity>();
            @Override
            public void onNext(ELFImgInfoMsg value) {
                if (jobid == null) {
                    if (value.hasJobId()) {
                        jobid=value.getJobId();
                    }else{
                        onCompleted();
                    }
                }
                ElfImgInfoEntity newEntity = new ElfImgInfoEntity(value);
                elfImgInfos.add(newEntity);

            }

            @Override
            public void onError(Throwable t) {

            }

            @Override
            public void onCompleted() {
                BinaryExecResult.Builder reply = BinaryExecResult.newBuilder();
                if (jobid == null) {
                    reply.setSuccess(false);
                    reply.setErrorMsg("Did not receive jobid in one of the elfImgInfo stream");
                } else {

                    Optional<JobEntity> _curJob = jobRepository.findById(jobid);
                    if (!_curJob.isPresent()) {
                        reply.setSuccess(false);
                        reply.setErrorMsg("Cannot find jobid=" + jobid.toString());
                    }
                    JobEntity curJob=_curJob.get();
                    curJob.setElfInfos(elfImgInfos);
                    JobEntity savedJob=jobRepository.save(curJob);
                    reply.setSuccess(true);
                    responseObserver.onNext(reply.build());
                }
                responseObserver.onCompleted();
            }
        };
    }


}
