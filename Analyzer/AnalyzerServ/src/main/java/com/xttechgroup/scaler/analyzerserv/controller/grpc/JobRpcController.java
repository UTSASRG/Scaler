package com.xttechgroup.scaler.analyzerserv.controller.grpc;

import com.google.protobuf.Empty;
import com.xttechgroup.scaler.analyzerserv.HelloMsg;
import com.xttechgroup.scaler.analyzerserv.JobGrpc;
import com.xttechgroup.scaler.analyzerserv.JobInfoMsg;
import com.xttechgroup.scaler.analyzerserv.models.Job.JobEntity;
import com.xttechgroup.scaler.analyzerserv.models.Job.JobRepo;
import io.grpc.stub.StreamObserver;
import net.devh.boot.grpc.server.service.GrpcService;
import org.springframework.beans.factory.annotation.Autowired;

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
}
