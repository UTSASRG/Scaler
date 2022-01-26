package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.nodes.ElfImgEntity;
import com.xttechgroup.scaler.analyzerserv.models.queries.ElfImgInfoQueryResult;
import org.springframework.data.neo4j.repository.Neo4jRepository;
import org.springframework.data.neo4j.repository.query.Query;

import java.util.List;

public interface ELFImgRepo extends Neo4jRepository<ElfImgEntity, Long> {
    /**
     * Get all ELF img info and symbol info
     *
     * @return
     */
    @Query("MATCH (curJob:Job)-[:HAS_IMGINFO]->(curImg:ElfImgInfo)\n" +
            "WHERE id(curJob)=0\n" +
            "CALL {\n" +
            "    WITH curImg\n" +
            "    MATCH (curImg)-[r:HAS_SYMINFO]->(curSym:ElfSymInfo)\n" +
            "    RETURN curSym,r\n" +
            "    LIMIT 20\n" +
            "}\n" +
            "// Counting symbols\n" +
            "MATCH (curImg)-[:HAS_SYMINFO]->(curSym1:ElfSymInfo)\n" +
            "RETURN curImg,r AS symbolsInThisFile,curSym,count(curSym1)  AS totalSymbols")
    List<ElfImgInfoQueryResult> getSymOfAnElfImg();


    /**
     * Get all ELF img info
     */
    @Query("MATCH (curJob:Job)-[:HAS_IMGINFO]->(curImg:ElfImgInfo)\n" +
            "WHERE id(curJob)=$jobID\n" +
            "MATCH (curImg)-[:HAS_SYMINFO]->(curSym:ElfSymInfo)" +
            "RETURN curImg,count(curSym) AS totalSymNumber")
    List<ElfImgInfoQueryResult> getELFImgEntity(Long jobID);


}

