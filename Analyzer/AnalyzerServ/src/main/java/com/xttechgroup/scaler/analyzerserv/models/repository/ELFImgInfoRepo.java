package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.nodes.info.ElfImgInfoEntity;
import org.springframework.data.neo4j.repository.Neo4jRepository;

public interface ELFImgInfoRepo extends Neo4jRepository<ElfImgInfoEntity, Long> {
}
