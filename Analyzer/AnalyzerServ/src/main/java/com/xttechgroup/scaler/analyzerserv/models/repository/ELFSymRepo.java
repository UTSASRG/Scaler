package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.nodes.ELFSymEntity;
import org.springframework.data.neo4j.repository.Neo4jRepository;
import org.springframework.data.neo4j.repository.query.Query;

import java.util.List;

public interface ELFSymRepo extends Neo4jRepository<ELFSymEntity, Long> {

    @Query("MATCH (curJob:Job)-[:HAS_IMG]->(curImg:ElfImg)\n" +
            "WHERE id(curJob)=$jobid\n" +
            "MATCH (curImg)-[:HAS_EXTSYM]->(curSym:ElfSym)\n" +
            "WHERE id(curImg)=$elfImgId\n" +
            "return curSym\n" +
            "SKIP $pagingStart\n" +
            "LIMIT $pagingNum\n"
            )
    List<ELFSymEntity> getELFSyms(Long jobid, Long elfImgId, Integer pagingNum, Integer pagingStart);

}
