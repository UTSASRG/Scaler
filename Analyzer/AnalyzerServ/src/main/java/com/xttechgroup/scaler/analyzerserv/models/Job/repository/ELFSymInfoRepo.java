package com.xttechgroup.scaler.analyzerserv.models.Job.repository;

import com.xttechgroup.scaler.analyzerserv.models.Job.nodes.ELFSymbolEntity;
import org.springframework.data.neo4j.repository.Neo4jRepository;

public interface ELFSymInfoRepo extends Neo4jRepository<ELFSymbolEntity, Long> {
}
