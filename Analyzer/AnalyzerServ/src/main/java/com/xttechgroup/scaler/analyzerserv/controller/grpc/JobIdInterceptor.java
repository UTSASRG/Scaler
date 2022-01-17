package com.xttechgroup.scaler.analyzerserv.controller.grpc;

import io.grpc.Metadata;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;

public class JobIdInterceptor implements ServerInterceptor {
    public static final Metadata.Key<String> METADATA_KEY = Metadata.Key.of("runid", Metadata.ASCII_STRING_MARSHALLER);

    @Override
    public <ReqT, RespT> ServerCall.Listener<ReqT> interceptCall(ServerCall<ReqT, RespT> call, Metadata headers, ServerCallHandler<ReqT, RespT> next) {
        String runid = headers.get(METADATA_KEY);
        return null;
    }
}
