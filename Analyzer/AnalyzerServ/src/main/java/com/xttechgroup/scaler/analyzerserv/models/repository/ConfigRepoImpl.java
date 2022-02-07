package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.ConfigGrpc;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.neo4j.core.Neo4jClient;
import org.springframework.stereotype.Component;

import java.util.Base64;

@Component
public class ConfigRepoImpl implements ConfigRepo {
    @Autowired
    Neo4jClient neo4jClient;

    @Override
    public String getConfigYaml(Long jobid) {
        return this.neo4jClient
                .query("MATCH (curJob:Job)\n" +
                        "WHERE id(curJob)=$jobid\n" +
                        "MATCH (curJob)-[:HAS_CONFIG]->(config:Config)\n" +
                        "return config.yamlConfig")
                .bind(jobid).to("jobid")
                .fetchAs(String.class)
                .mappedBy((typeSystem, record) ->
                        record.get(0).asString().replace("&#13","\n")
                )
                .first().get();
    }

    @Override
    public void setConfigYaml(Long jobid, String yamlConfig) {
        this.neo4jClient
                .query("MATCH (curJob:Job)\n" +
                        "WHERE id(curJob)=$jobid\n" +
                        "MATCH (curJob)-[:HAS_CONFIG]->(config:Config)\n" +
                        "SET config.yamlConfig=$yamlConfig\n" +
                        "return config.yamlConfig")
                .bind(yamlConfig).to("yamlConfig")
                .bind(jobid).to("jobid")
                .fetchAs(String.class)
                .mappedBy((typeSystem, record) -> record.get(0).asString())
                .first().get();
    }
}
