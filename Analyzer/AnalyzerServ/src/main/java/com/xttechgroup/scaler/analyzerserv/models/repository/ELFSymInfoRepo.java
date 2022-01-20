package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.nodes.info.ELFSymInfoEntity;
import org.springframework.data.neo4j.repository.Neo4jRepository;

public interface ELFSymInfoRepo extends Neo4jRepository<ELFSymInfoEntity, Long> {
}
