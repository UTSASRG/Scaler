package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.nodes.ELFSymEntity;
import org.springframework.data.neo4j.repository.Neo4jRepository;
import org.springframework.data.neo4j.repository.query.Query;

import java.util.List;

public interface ELFSymRepo extends Neo4jRepository<ELFSymEntity, Long> {

    @Query("MATCH (curJob:Job)-[:HAS_IMGINFO]->(curImg:ElfImgInfo)\n" +
            "WHERE id(curJob)=$jobid\n" +
            "MATCH (curImg)-[:HAS_SYMINFO]->(curSym:ElfSymInfo)\n" +
            "WHERE id(curImg)=$elfImgId\n" +
            "return curSym\n" +
            "LIMIT $pagingNum")
    List<ELFSymEntity> getELFSyms(Long jobid, Long elfImgId, Integer pagingNum);
}
