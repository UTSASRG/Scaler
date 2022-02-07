package com.xttechgroup.scaler.analyzerserv.controller.restful.config;

import com.xttechgroup.scaler.analyzerserv.models.POJO.ExecutionCommand;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.locks.Lock;

@Configuration
public class WaitForConfigMap {
    static Map<Long, ExecutionCommand> _configRequestMap=Collections.synchronizedMap(new HashMap<>());
    @Bean
    public Map<Long, ExecutionCommand> configRequestMap() {
        return _configRequestMap;
    }



}
