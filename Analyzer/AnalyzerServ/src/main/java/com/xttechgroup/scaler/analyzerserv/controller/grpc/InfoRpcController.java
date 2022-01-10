package com.xttechgroup.scaler.analyzerserv.controller.grpc;

import com.google.protobuf.Empty;
import com.xttechgroup.scaler.analyzerserv.HelloMsg;
import com.xttechgroup.scaler.analyzerserv.InfoGrpc;
import io.grpc.stub.StreamObserver;
import net.devh.boot.grpc.server.service.GrpcService;
import org.springframework.beans.factory.annotation.Value;

@GrpcService
public class InfoRpcController extends InfoGrpc.InfoImplBase {
    private final String api = "rest";
    @Value("${app.name}")
    private String appName;
    @Value("${app.version}")
    private String version;
    private final String slogan = "You know, for performance.";

    @Override
    public void sayHello(Empty request, StreamObserver<HelloMsg> responseObserver) {
        HelloMsg reply = HelloMsg.newBuilder()
                .setApi(api)
                .setAppName(appName)
                .setAppVersion(version)
                .setSlogan(slogan)
                .build();
        responseObserver.onNext(reply);
        responseObserver.onCompleted();
    }
}
