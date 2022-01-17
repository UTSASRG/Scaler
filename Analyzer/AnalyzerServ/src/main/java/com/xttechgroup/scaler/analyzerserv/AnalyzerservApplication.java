package com.xttechgroup.scaler.analyzerserv;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.context.ConfigurableApplicationContext;
import org.springframework.data.neo4j.repository.config.EnableNeo4jRepositories;

import java.util.Map;

@SpringBootApplication
@EnableNeo4jRepositories
public class AnalyzerservApplication {

    public static void main(String[] args) {
        SpringApplication.run(AnalyzerservApplication.class, args);
    }

}


