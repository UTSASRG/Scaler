package com.xttechgroup.scaler.analyzerserv.controller.grpc;

import com.google.common.util.concurrent.MoreExecutors;
import com.xttechgroup.scaler.analyzerserv.*;
import com.xttechgroup.scaler.analyzerserv.models.POJO.ExecutionCommand;
import io.grpc.Context;
import io.grpc.stub.StreamObserver;
import net.devh.boot.grpc.server.service.GrpcService;
import org.neo4j.driver.Driver;
import org.neo4j.driver.Record;
import org.neo4j.driver.Session;
import org.springframework.beans.factory.annotation.Autowired;

import java.util.Base64;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

@GrpcService
public class ConfigRPCController extends ConfigGrpc.ConfigImplBase {
    @Autowired
    Driver neo4jDriver;
    @Autowired
    Map<Long, ExecutionCommand> configRequestMap;

    @Override
    public void appendYAMLConfig(ConfigMsg request, StreamObserver<BinaryExecResult> responseObserver) {
        BinaryExecResult.Builder reply = BinaryExecResult.newBuilder();
        try (Session session = neo4jDriver.session()) {
            session.writeTransaction(tx ->
            {
                //Bulk insert elf img
                String query =
                        //Expand $jobInvokeSymInfos
                        "MATCH (curJob:Job)\n" +
                                "WHERE ID(curJob)=$jobId \n" +
                                "MERGE (curJob)-[:HAS_CONFIG]->(config:Config)" + "\n" +
                                "SET config.yamlConfig=$yamlConfig\n" +
                                "RETURN id(config)\n";
                Map<String, Object> params = new HashMap<>();
                params.put("yamlConfig", request.getYamlConfig().replace("\n","&#13"));

                params.put("jobId", request.getJobId());

                Long configId = tx.run(query, params).next().get(0).asLong();


                if (configId <= 0) {
                    reply.setSuccess(false);
                    reply.setErrorMsg("Saving yaml config failed.");
                    tx.rollback();
                } else {
                    reply.setSuccess(true);
                }

                return null;
            });

        }
        responseObserver.onNext(reply.build());
        responseObserver.onCompleted();

    }

    @Override
    public void getConfigFromServer(JobInfoMsg request, StreamObserver<YAMLConfigRlt> responseObserver) {
        Long jobId = request.getId();
        YAMLConfigRlt.Builder yamlConfigRlt=YAMLConfigRlt.newBuilder();
        Context.current().withCancellation().addListener(new Context.CancellationListener() {
            @Override
            public void cancelled(Context context) {
                configRequestMap.remove(jobId);
            }
        }, MoreExecutors.directExecutor());
        ExecutionCommand signalObj = new ExecutionCommand();
        configRequestMap.put(jobId, signalObj);
        try {
            synchronized (signalObj) {
                signalObj.wait();
                if(signalObj.execute){
                    yamlConfigRlt.setSuccess(true);
                    yamlConfigRlt.setYamlConfig(signalObj.yamlConfig);
                }else{
                    yamlConfigRlt.setSuccess(false);
                    yamlConfigRlt.setErrorMsg("User cancelled this execution");
                }
            }
        } catch (InterruptedException e) {
            configRequestMap.remove(jobId);
            e.printStackTrace();
        }
        responseObserver.onNext(yamlConfigRlt.build());
        responseObserver.onCompleted();

    }
}
