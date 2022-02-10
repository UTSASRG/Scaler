package com.xttechgroup.scaler.analyzerserv.controller.grpc;

import com.google.protobuf.Empty;
import com.xttechgroup.scaler.analyzerserv.*;
import com.xttechgroup.scaler.analyzerserv.models.nodes.ELFSymEntity;
import com.xttechgroup.scaler.analyzerserv.models.nodes.ElfImgEntity;
import com.xttechgroup.scaler.analyzerserv.models.nodes.JobEntity;
import com.xttechgroup.scaler.analyzerserv.models.repository.ELFImgRepo;
import com.xttechgroup.scaler.analyzerserv.models.repository.ELFSymRepo;
import com.xttechgroup.scaler.analyzerserv.models.repository.JobRepo;
import io.grpc.stub.StreamObserver;
import net.devh.boot.grpc.server.service.GrpcService;
import org.neo4j.driver.*;
import org.springframework.beans.factory.annotation.Autowired;

import java.util.*;
import java.util.concurrent.locks.Lock;

@GrpcService
public class JobRpcController extends JobGrpc.JobImplBase {
    @Autowired
    JobRepo jobRepository;
    @Autowired
    ELFSymRepo elfSymInfoRepo;
    @Autowired
    ELFImgRepo elfImgInfoRepo;
    @Autowired
    Driver neo4jDriver;


    @Override
    public void createJob(Empty request, StreamObserver<JobInfoMsg> responseObserver) {
        Long jobId = jobRepository.newJob();
        JobInfoMsg reply = JobInfoMsg.newBuilder()
                .setId(jobId)
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
                        curJob = jobRepository.findById(jobid);
                        if (curJob == null) {
                            onCompleted();
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
                                    "CREATE (newImgNode:ElfImg) <-[:HAS_IMG]- (curJob)" + "\n" +
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
                                                "MATCH (curImg:ElfImg)\n" +
                                                "WHERE ID(curImg)=$insertedImgId[symNode.elfImgId]\n" +
                                                "CREATE (newSym:ElfSym) <-[:HAS_EXTSYM]- (curImg)" + "\n" +
                                                "SET newSym.symbolName =symNode.symbolName, " +
                                                "newSym.scalerId=symNode.scalerId, " +
                                                "newSym.symbolType=symNode.symbolType, " +
                                                "newSym.bindType=symNode.bindType, " +
                                                "newSym.libFileId=symNode.libFileId, " +
                                                "newSym.gotAddr=symNode.gotAddr," +
                                                "newSym.hooked=symNode.hooked, " +
                                                "newSym.symbolName=symNode.symbolName, " +
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
        ArrayList<Map<String, Object>> extSymInvokeImgInfos = new ArrayList<>();
        BinaryExecResult.Builder reply = BinaryExecResult.newBuilder();
        try (Session session = neo4jDriver.session()) {
            session.writeTransaction(tx ->
            {
                long processId = timingMsg.getProcessId();
                long threadId = timingMsg.getThreadId();
                long timingMatrixRows = timingMsg.getTimgMatrixrows();
                long timingMatrixCols = timingMsg.getTimgMatrixcols();
                long countingVecRows = timingMsg.getCountingVecRows();

                //Insert JobInvokeSym info
                for (int symHookID = 0; symHookID < countingVecRows; ++symHookID) {
                    Map<String, Object> curInvokedSymInfo = new HashMap<>();
                    long counts = timingMsg.getCountingVecVal(symHookID);
                    if (counts > 0) {
                        curInvokedSymInfo.put("counts", counts);
                        curInvokedSymInfo.put("symInfo", symHookID);
                        jobInvokeSymInfos.add(curInvokedSymInfo);
                    }
                }

                //Insert ImgInvokeSym info
                assert timingMsg.getTimgMatrixrows() == timingMsg.getCountingVecRows();
                for (int symHookedID = 0; symHookedID < timingMatrixRows; ++symHookedID) {
                    for (int libFileScalerID = 0; libFileScalerID < timingMatrixCols; ++libFileScalerID) {
                        Map<String, Object> curInvokedSymInfo = new HashMap<>();
                        long duration = timingMsg.getTimgMatrixVal((int) (symHookedID * timingMatrixCols + libFileScalerID));
                        //todo: Maybe using int is better?
                        if (duration > 0) {
                            curInvokedSymInfo.put("duration", duration);
                            curInvokedSymInfo.put("symHookedID", symHookedID);
                            curInvokedSymInfo.put("libFileScalerID", libFileScalerID);
                            extSymInvokeImgInfos.add(curInvokedSymInfo);
                            //System.out.println(symHookedID + " called " + libFileScalerID );
                        }
                    }
                }

                //Then, insert all nodes in one request
                Map<String, Object> params = new HashMap<>();
                params.put("jobInvokeSymInfos", jobInvokeSymInfos);
                params.put("extSymInvokeImgInfos", extSymInvokeImgInfos);
                params.put("jobId", timingMsg.getJobId());
                params.put("threadId", threadId);
                params.put("processId", processId);

                //Bulk insert elf img
                String query =
                        //Expand $jobInvokeSymInfos
                        "MATCH (curJob:Job)\n" +
                                "WHERE ID(curJob)=$jobId \n" +
                                "UNWIND $jobInvokeSymInfos AS jobInvokeSymInfo" + "\n" +
                                "MATCH (curJob)-[:HAS_IMG]->(:ElfImg)-[:HAS_EXTSYM]->(targetSymbol:ElfSym)\n" +
                                "USING INDEX targetSymbol:ElfSym(hookedId)\n" +
                                "WHERE targetSymbol.hookedId=jobInvokeSymInfo.symInfo\n" +
                                //Insert JobInvokeSym relation
                                "CREATE (curJob)-[jobInvokeSym:JobInvokeSym {counts:jobInvokeSymInfo.counts,threadId:$threadId,processId:$processId}]-> (targetSymbol)\n" +
                                "RETURN jobInvokeSym\n";
                List<Record> result = tx.run(query, params).list();
                if (result.size() != jobInvokeSymInfos.size()) {
                    reply.setSuccess(false);
                    reply.setErrorMsg("Saving JobInvokeSym failed.");
                    tx.rollback();
                } else {

                    query = "UNWIND $extSymInvokeImgInfos AS extSymInvokeImgInfo\n" +
                            "MATCH (curJob:Job)-[:HAS_IMG]->(calleeImg:ElfImg)\n" +
                            "WHERE id(curJob)=$jobId AND calleeImg.scalerId=extSymInvokeImgInfo.libFileScalerID\n" +
                            "MATCH (curJob:Job)-[:HAS_IMG]->(callerImg:ElfImg)-[:HAS_EXTSYM]->(invokedSym:ElfSym)\n" +
                            "USING INDEX invokedSym:ElfSym(hookedId)\n" +
                            "WHERE id(curJob)=$jobId AND invokedSym.hookedId=extSymInvokeImgInfo.symHookedID\n" +
                            "CREATE (invokedSym)-[r:ExtSymInvokeImg {duration:extSymInvokeImgInfo.duration,threadId:$threadId,processId:$processId}]->(calleeImg)\n" +
                            "RETURN r\n";
                    result = tx.run(query, params).list();
                    if (result.size() != extSymInvokeImgInfos.size()) {
                        reply.setSuccess(false);
                        reply.setErrorMsg("Saving JobInvokeSym failed.");
                        tx.rollback();
                    } else {
                        reply.setSuccess(true);
                    }
                }

                return null;
            });

        }


        responseObserver.onNext(reply.build());
        responseObserver.onCompleted();

    }

    @Override
    public void appendThreadExecTime(ThreadTotalTimeMsg request, StreamObserver<BinaryExecResult> responseObserver) {
        BinaryExecResult.Builder reply = BinaryExecResult.newBuilder();
        try (Session session = neo4jDriver.session()) {
            session.writeTransaction(tx ->
            {
                //Then, insert all nodes in one request
                Map<String, Object> params = new HashMap<>();
                params.put("jobId", request.getJobId());
                params.put("threadId", request.getThreadId());
                params.put("processId", request.getProcessId());
                params.put("totalTime", request.getTotalTime());

                //Bulk insert elf img
                String query =
                        //Expand $jobInvokeSymInfos
                        "MATCH (curJob:Job)\n" +
                                "WHERE ID(curJob)=$jobId \n" +
                                "CREATE (curJob)-[:HAS_THREAD]->(curThread:Thread)\n" +
                                "SET curThread.totalTime=$totalTime\n" +
                                "SET curThread.threadId=$threadId\n" +
                                "SET curThread.processId=$processId\n" +
                                "RETURN count(curThread)\n";
                List<Record> result = tx.run(query, params).list();
                if (result.size() != 1) {
                    reply.setSuccess(false);
                    reply.setErrorMsg("Saving thread execution time failed.");
                    tx.rollback();
                } else {

                    reply.setSuccess(true);
                }
                return null;

            });
        }
    }
}
