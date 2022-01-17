package com.xttechgroup.scaler.analyzerserv.models.Job;

import org.neo4j.driver.internal.shaded.reactor.core.publisher.Mono;
import org.springframework.data.neo4j.repository.Neo4jRepository;
import org.springframework.data.neo4j.repository.query.Query;

public interface JobRepo extends Neo4jRepository<JobEntity, Long> {
}
