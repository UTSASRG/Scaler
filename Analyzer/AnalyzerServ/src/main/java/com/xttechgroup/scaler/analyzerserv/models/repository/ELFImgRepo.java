package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.nodes.ElfImgEntity;
import org.springframework.data.neo4j.repository.Neo4jRepository;
import org.springframework.data.neo4j.repository.query.Query;

import java.util.List;

public interface ELFImgRepo extends Neo4jRepository<ElfImgEntity, Long> {

    /**
     * Get all ELF img info
     */
    @Query("MATCH (curJob:Job)-[:HAS_IMG]->(curImg:ElfImg)\n" +
            "WHERE id(curJob)=$jobID AND (curImg.elfImgValid=$elfImgValid OR ($elfImgValid is null))\n" +
            "RETURN curImg\n"
    )
    List<ElfImgEntity> getELFImgEntity(Long jobID, Boolean elfImgValid);

    @Query("MATCH (curJob:Job)-[:HAS_IMG]->(curImg:ElfImg)\n" +
            "WHERE id(curJob)=$jobID AND id(curImg)=$imgId\n" +
            "MATCH (curImg)-[:HAS_EXTSYM]->(curSym:ElfSym)\n" +
            "WHERE  curSym.hooked=true\n" +
            "RETURN count(curSym)")
    Long getHookedSymSum(Long jobID, Long imgId);

    @Query("MATCH (curJob:Job)-[:HAS_IMG]->(curImg:ElfImg)\n" +
            "WHERE id(curJob)=$jobID AND id(curImg)=$imgId\n" +
            "MATCH (curImg)-[:HAS_EXTSYM]->(curSym:ElfSym)\n" +
            "RETURN count(curSym)\n")
    Long getAllSymSum(Long jobID, Long imgId);

    @Query("UNWIND $elfImgIds AS elfImgId\n" +
            "MATCH (curJob:Job)-[p:JobInvokeSym]->(curSym:ElfSym)\n" +
            "WHERE id(curJob)=$jobid\n" +
            "MATCH (curSym)<-[:HAS_EXTSYM]-(curImg:ElfImg)\n" +
            "WHERE id(curImg)=elfImgId\n" +
            "RETURN curSym")
    List<Long> getELFImgInvokedSym(Long jobid, Long elfImgIds);
}

