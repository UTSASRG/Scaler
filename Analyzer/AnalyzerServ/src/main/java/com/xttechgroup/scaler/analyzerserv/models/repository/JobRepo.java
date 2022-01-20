package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.nodes.JobEntity;
import org.springframework.data.neo4j.repository.Neo4jRepository;

public interface JobRepo extends Neo4jRepository<JobEntity, Long> {
}
