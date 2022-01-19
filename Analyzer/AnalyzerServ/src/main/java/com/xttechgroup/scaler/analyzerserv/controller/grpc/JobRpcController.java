package com.xttechgroup.scaler.analyzerserv.controller.grpc;

import com.google.protobuf.Empty;
import com.xttechgroup.scaler.analyzerserv.*;
import com.xttechgroup.scaler.analyzerserv.models.Job.*;
import io.grpc.Metadata;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;
import io.grpc.stub.StreamObserver;
import net.devh.boot.grpc.server.service.GrpcService;
import org.neo4j.driver.Driver;
import org.neo4j.driver.Result;
import org.neo4j.driver.Session;
import org.springframework.beans.factory.annotation.Autowired;

import java.util.*;

import static org.neo4j.driver.Values.parameters;

@GrpcService
public class JobRpcController extends JobGrpc.JobImplBase {
    @Autowired
    JobRepo jobRepository;
    @Autowired
    ELFSymInfoRepo elfSymInfoRepo;
    @Autowired
    ELFImgInfoRepo elfImgInfoRepo;
    @Autowired
    Driver neo4jDriver;

    @Override
    public void createJob(Empty request, StreamObserver<JobInfoMsg> responseObserver) {
        JobEntity newJob = new JobEntity();
        newJob = jobRepository.save(newJob);
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
            JobEntity curJob = null;
            ArrayList<ElfImgInfoEntity> imgNodes = new ArrayList<>();
            ArrayList<ELFSymbolEntity> symNodes = new ArrayList<>();

            @Override
            public void onNext(ELFImgInfoMsg value) {
                if (jobid == null) {
                    if (value.hasJobId()) {
                        jobid = value.getJobId();
                        Optional<JobEntity> _curJob = jobRepository.findById(jobid);
                        if (!_curJob.isPresent()) {
                            curJob = null;
                            onCompleted();
                        } else {
                            curJob = _curJob.get();
                        }
                    } else {
                        onCompleted();
                    }
                }

                ElfImgInfoEntity newEntity = new ElfImgInfoEntity(curJob, value);
                imgNodes.add(newEntity);
                newEntity = elfImgInfoRepo.save(newEntity);
                System.out.println("id:"+newEntity.id);
                List<Map<String, Object>> symbolInfoRaw = new ArrayList<>();
                long startTime = System.currentTimeMillis();

                for (ELFSymbolInfoMsg symbolInfoMsg : value.getSymbolInfoInThisFileList()) {
                    symNodes.add(new ELFSymbolEntity(newEntity, symbolInfoMsg));

                    Map<String, Object> n1 = new HashMap<>();


                    n1.put("scalerId", symbolInfoMsg.getScalerId());
                    n1.put("symbolType", symbolInfoMsg.getSymbolType().name());
                    n1.put("bindType", symbolInfoMsg.getBindType().name());
                    n1.put("libFileId", symbolInfoMsg.getLibFileId());
                    n1.put("gotAddr", symbolInfoMsg.getGotAddr());
                    n1.put("hooked", symbolInfoMsg.getHooked());
                    n1.put("symbolName", symbolInfoMsg.getSymbolName());
                    n1.put("elfImgInfoEntity", newEntity.id);

                    symbolInfoRaw.add(n1);
                }
                //elfSymInfoRepo.saveAll(symNodes);
                try (Session session = neo4jDriver.session()) {
                    session.writeTransaction(tx ->
                    {
                        Map<String, Object> params = new HashMap<>();
                        params.put("mantdata", symbolInfoRaw);
                        String query =
                                "UNWIND $mantdata AS data" + "\n" +
                                        "MATCH (parentImg:ElfImgInfo)\n"+
                                        "WHERE ID(parentImg)=data.elfImgInfoEntity\n"+
                                        "CREATE (n:ELFSymbol) <-[:HAS_SYM]- (parentImg)" + "\n" +
                                        "SET n.symbolName =data.symbolName, n.symbolType=data.symbolType, n.imgid=data.elfImgInfoEntity";

                        Result result = tx.run(query, params);
                        return null;
                    });
                }
                long endTime = System.currentTimeMillis();
                System.out.println((endTime-startTime)/1000.0+"s");

                symNodes.clear();
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
                } else if (curJob == null) {
                    reply.setSuccess(false);
                    reply.setErrorMsg("Cannot find jobid=" + jobid.toString());
                } else {
//                    System.out.println("Saving imgnodes");
//                    List<ELFImgInfoRepo> elfImgInfoRepo.saveAll(imgNodes);
//                    System.out.println("Saving symnodes" + symNodes.size());
////                    elfSymInfoRepo.saveAll(symNodes);
//
//
//                    try (Session session = neo4jDriver.session()) {
//                        System.out.println("Here");
//                    }

                    reply.setSuccess(true);
                    responseObserver.onNext(reply.build());
                }
                responseObserver.onCompleted();
            }
        };
    }


}
