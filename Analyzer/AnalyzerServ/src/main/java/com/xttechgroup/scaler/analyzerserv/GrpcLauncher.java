package com.xttechgroup.scaler.analyzerserv;

import io.grpc.BindableService;
import io.grpc.Server;
import io.grpc.ServerBuilder;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import java.io.IOException;
import java.util.Map;

//Reference: https://zhuanlan.zhihu.com/p/46268200
@Component("grpcLauncher")
public class GrpcLauncher {
    /**
     * Grpc Server
     */
    private Server server;

    @Value("${grpc.server.port}")
    private Integer grpcServerPort;

    public void grpcStart(Map<String, Object> grpcServiceBeanMap) {
        try {
            ServerBuilder serverBuilder = ServerBuilder.forPort(grpcServerPort);
            for (Object bean : grpcServiceBeanMap.values()) {
                serverBuilder.addService((BindableService) bean);
                System.out.println(bean.getClass().getSimpleName() + " is regist in Spring Boot");
            }
            server = serverBuilder.build().start();
            System.out.println("grpc server is started at " + grpcServerPort);
            server.awaitTermination();
            Runtime.getRuntime().addShutdownHook(new Thread(() -> {
                grpcStop();
            }));
        } catch (IOException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    private void grpcStop() {
        if (server != null) {
            server.shutdownNow();
        }
    }
}
