package com.xttechgroup.scaler.analyzerserv.controller.grpc;

import com.google.protobuf.Empty;
import com.xttechgroup.scaler.analyzerserv.*;
import com.xttechgroup.scaler.analyzerserv.models.nodes.ELFSymEntity;
import com.xttechgroup.scaler.analyzerserv.models.nodes.ElfImgEntity;
import com.xttechgroup.scaler.analyzerserv.models.nodes.JobEntity;
import com.xttechgroup.scaler.analyzerserv.models.repository.ELFImgInfoRepo;
import com.xttechgroup.scaler.analyzerserv.models.repository.ELFSymInfoRepo;
import com.xttechgroup.scaler.analyzerserv.models.repository.JobRepo;
import io.grpc.stub.StreamObserver;
import net.devh.boot.grpc.server.service.GrpcService;
import org.neo4j.driver.*;
import org.neo4j.driver.summary.ResultSummary;
import org.springframework.beans.factory.annotation.Autowired;

import java.util.*;

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
            ArrayList<Map<String, Object>> imgNodes = new ArrayList<>();
            ArrayList<Map<String, Object>> symNodes = new ArrayList<>();

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

                imgNodes.add(ElfImgEntity.protoToMap(value, jobid));

                for (ELFSymbolInfoMsg symbolInfoMsg : value.getSymbolInfoInThisFileList()) {
                    symNodes.add(ELFSymEntity.protoToMap(symbolInfoMsg, imgNodes.size() - 1));
                }
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
                    //elfSymInfoRepo.saveAll(symNodes);
                    try (Session session = neo4jDriver.session()) {

                        session.writeTransaction(tx ->
                        {

                            Map<String, Object> params = new HashMap<>();
                            params.put("imgNodes", imgNodes);
                            //Bulk insert elf img
                            String query = "UNWIND $imgNodes AS imgNode" + "\n" +
                                    "MATCH (curJob:Job)\n" +
                                    "WHERE ID(curJob)=imgNode.jobId\n" +
                                    "CREATE (newImgNode:ElfImgInfo) <-[:HAS_IMGINFO]- (curJob)" + "\n" +
                                    "SET newImgNode.scalerId =imgNode.scalerId, " +
                                    "newImgNode.filePath=imgNode.filePath, " +
                                    "newImgNode.elfImgValid=imgNode.elfImgValid, " +
                                    "newImgNode.symbolType=imgNode.symbolType, " +
                                    "newImgNode.addrStart=imgNode.addrStart, " +
                                    "newImgNode.addrEnd=imgNode.addrEnd," +
                                    "newImgNode.pltStartAddr=imgNode.pltStartAddr, " +
                                    "newImgNode.pltSecStartAddr=imgNode.pltSecStartAddr\n" +
                                    "return id(newImgNode)";
                            List<Record> rltImg = tx.run(query, params).list();

                            if (rltImg.size() != imgNodes.size()) {
                                reply.setSuccess(false);
                                reply.setErrorMsg("Saving image node failed, not every save was successful.");
                                tx.rollback();
                            } else {

                                List<Long> insertedImgId = new ArrayList<>();
                                for (int i = 0; i < rltImg.size(); ++i) {
                                    insertedImgId.add(rltImg.get(i).get("id(newImgNode)").asLong());
                                }

                                params.put("insertedImgId", insertedImgId);
                                params.put("symNodes", symNodes);
                                query =
                                        "UNWIND $symNodes AS symNode" + "\n" +
                                                "MATCH (curImg:ElfImgInfo)\n" +
                                                "WHERE ID(curImg)=$insertedImgId[symNode.elfImgId]\n" +
                                                "CREATE (newSym:ElfSymInfo) <-[:HAS_SYMINFO]- (curImg)" + "\n" +
                                                "SET newSym.symbolName =symNode.symbolName, " +
                                                "newSym.symbolType=symNode.symbolType, " +
                                                "newSym.bindType=symNode.bindType, " +
                                                "newSym.libFileId=symNode.libFileId, " +
                                                "newSym.gotAddr=symNode.gotAddr," +
                                                "newSym.hooked=symNode.hooked, " +
                                                "newSym.symbolName=symNode.symbolName, " +
                                                "newSym.elfImgInfoEntity=symNode.elfImgInfoEntity," +
                                                "newSym.hookedId=symNode.hookedId";


                                Result result = tx.run(query, params);
                                if (rltImg.size() != imgNodes.size()) {
                                    reply.setErrorMsg("Saving symbol node failed, not every save was successful");
                                    tx.rollback();
                                }
                            }
                            return null;
                        });
                    }

                    reply.setSuccess(true);
                }
                responseObserver.onNext(reply.build());
                responseObserver.onCompleted();
            }
        };
    }


    @Override
    public void appendTimingMatrix(TimingMsg timingMsg, StreamObserver<BinaryExecResult> responseObserver) {
        ArrayList<Map<String, Object>> jobInvokeSymInfos = new ArrayList<>();
        ArrayList<Map<String, Object>> imgInvokeSymInfos = new ArrayList<>();
        BinaryExecResult.Builder reply = BinaryExecResult.newBuilder();
        try (Session session = neo4jDriver.session()) {
            session.writeTransaction(tx ->
            {
                long timingMatrixRows = timingMsg.getTimgMatrixrows();
                long timingMatrixCols = timingMsg.getTimgMatrixcols();
                long countingVecRows = timingMsg.getCountingVecRows();

                //Insert JobInvokeSym info
                for (int symHookID = 0; symHookID < countingVecRows; ++symHookID) {
                    Map<String, Object> curInvokedSymInfo = new HashMap<>();
                    curInvokedSymInfo.put("counts", timingMsg.getCountingVecVal(symHookID));
                    curInvokedSymInfo.put("symInfo", symHookID);
                    jobInvokeSymInfos.add(curInvokedSymInfo);

                }

                //Insert ImgInvokeSym info
                assert timingMsg.getTimgMatrixrows() == timingMsg.getCountingVecRows();
                for (int libFileId = 0; libFileId < timingMatrixRows; ++libFileId) {
                    for (int symHookedID = 0; symHookedID < timingMatrixCols; ++symHookedID) {
                        Map<String, Object> curInvokedSymInfo = new HashMap<>();
                        //todo: Maybe using int is better?
                        curInvokedSymInfo.put("duration", timingMsg.getTimgMatrixVal((int) (libFileId * timingMatrixRows + symHookedID)));
                        curInvokedSymInfo.put("libFileId", libFileId);
                        curInvokedSymInfo.put("symHookedID", symHookedID);
                        imgInvokeSymInfos.add(curInvokedSymInfo);
                    }
                }


                //Then, insert all nodes in one request
                Map<String, Object> params = new HashMap<>();
                params.put("jobInvokeSymInfos", jobInvokeSymInfos);
                params.put("imgInvokeSymInfos", imgInvokeSymInfos);
                params.put("jobId", timingMsg.getJobId());


                //Bulk insert elf img
                String query =
                        //Expand $jobInvokeSymInfos
                        "MATCH (curJob:Job)\n" +
                                "WHERE ID(curJob)=$jobId \n" +
                                "UNWIND $jobInvokeSymInfos AS jobInvokeSymInfo" + "\n" +
                                "MATCH (curJob)-[]->()-[]->(targetSymbol:ElfSymInfo)\n" +
                                "USING INDEX targetSymbol:ElfSymInfo(hookedId)\n"+
                                "WHERE targetSymbol.hookedId=jobInvokeSymInfo.symInfo\n" +
                                //Insert JobInvokeSym relation
                                "CREATE (curJob)-[jobInvokeSym:JobInvokeSym {counts:jobInvokeSymInfo.counts}]-> (targetSymbol)\n" +
                                "return jobInvokeSym\n";
                List<Record> result = tx.run(query, params).list();

                if (result.size() != jobInvokeSymInfos.size()) {
                    reply.setSuccess(false);
                    reply.setErrorMsg("Saving JobInvokeSym failed.");
                    tx.rollback();
                } else {

                    //Expand $jobInvokeSymInfos
//                    query = "UNWIND $imgInvokeSymInfos AS imgInvokeSymInfo" + "\n" +
//                            //Find image
//                            "MATCH (curImg:ElfImgInfo)\n" +
//                            "WHERE curImg.scalerId=imgInvokeSymInfo.symInfo\n" +
//                            //Find symbol
//                            "MATCH (imgInvokedSym: ElfSymInfo)\n" +
//                            "WHERE imgInvokedSym.hookedId=imgInvokeSymInfo.symInfo\n" +
//                            //Insert ImgInvokeSym relation
//                            "CREATE (curImg)-[imgInvokeSym:ImgInvokeSym {duration:imgInvokeSymInfo.duration}]-> (imgInvokedSym)\n";
//                    rltImg = tx.run(query, params).list();
//                    if (rltImg.size() != imgInvokeSymInfos.size()) {
//                        reply.setSuccess(false);
//                        reply.setErrorMsg("Saving ImgInvokeSym failed.");
//                        tx.rollback();
//                    }
                }

                return null;
            });

        }


        responseObserver.onNext(reply.build());
        responseObserver.onCompleted();

    }
}
