package com.xttechgroup.scaler.analyzerserv.models.Job.repository;

import com.xttechgroup.scaler.analyzerserv.models.Job.nodes.ElfImgInfoEntity;
import org.springframework.data.neo4j.repository.Neo4jRepository;

public interface ELFImgInfoRepo extends Neo4jRepository<ElfImgInfoEntity, Long> {
}
