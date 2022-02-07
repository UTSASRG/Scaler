package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.nodes.JobEntity;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.neo4j.core.Neo4jClient;
import org.springframework.stereotype.Component;

@Component
public class JobRepoImpl implements JobRepo {
    @Autowired
    Neo4jClient neo4jClient;

    @Override
    public Long newJob() {
        return this.neo4jClient
                .query("CREATE (curJob:Job)\n" +
                        "RETURN id(curJob)")
                .fetchAs(Long.class)
                .mappedBy((typeSystem, record) -> record.get(0).asLong())
                .first().get();
    }

    @Override
    public JobEntity findById(Long jobId) {
        return this.neo4jClient
                .query("MATCH (curJob:Job)\n" +
                        "WHERE id(curJob)=$jobId\n" +
                        "RETURN id(curJob)")
                .bind(jobId).to("jobId")
                .fetchAs(JobEntity.class)
                .mappedBy((typeSystem, record) -> {
                    JobEntity entity = new JobEntity();
                    entity.id = record.get(0).asLong();
                    return entity;
                })
                .first().get();
    }
}
