package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.nodes.ElfImgEntity;
import com.xttechgroup.scaler.analyzerserv.models.queries.ElfImgInfoQueryResult;
import org.springframework.data.neo4j.repository.Neo4jRepository;
import org.springframework.data.neo4j.repository.query.Query;

import java.util.List;

public interface ELFImgRepo extends Neo4jRepository<ElfImgEntity, Long> {

    /**
     * Get all ELF img info
     */
    @Query("MATCH (curJob:Job)-[:HAS_IMGINFO]->(curImg:ElfImgInfo)\n" +
            "WHERE id(curJob)=$jobID\n" +
            "RETURN curImg\n"
    )
    List<ElfImgEntity> getELFImgEntity(Long jobID);

    @Query("MATCH (curJob:Job)-[:HAS_IMGINFO]->(curImg:ElfImgInfo)\n" +
            "WHERE id(curJob)=$jobID AND id(curImg)=$imgId\n" +
            "MATCH (curImg)-[:HAS_SYMINFO]->(curSym:ElfSymInfo)\n" +
            "WHERE  curSym.hooked=true\n" +
            "RETURN count(curSym)")
    Long getHookedSymSum(Long jobID, Long imgId);

    @Query("MATCH (curJob:Job)-[:HAS_IMGINFO]->(curImg:ElfImgInfo)\n" +
            "WHERE id(curJob)=$jobID AND id(curImg)=$imgId\n" +
            "MATCH (curImg)-[:HAS_SYMINFO]->(curSym:ElfSymInfo)\n" +
            "RETURN count(curSym)\n")
    Long getAllSymSum(Long jobID, Long imgId);

}

