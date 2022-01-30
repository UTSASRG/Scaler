package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.nodes.ELFSymEntity;
import com.xttechgroup.scaler.analyzerserv.models.nodes.JobEntity;
import com.xttechgroup.scaler.analyzerserv.models.POJO.JobInvokedSymQueryResult;
import com.xttechgroup.scaler.analyzerserv.models.relations.JobInvokeSym;
import org.springframework.data.neo4j.repository.Neo4jRepository;
import org.springframework.data.neo4j.repository.query.Query;

import java.util.List;

public interface JobInvokedSymRepo extends Neo4jRepository<JobInvokeSym, Long> {


    @Query("MATCH (curJob:Job)\n" +
            "WHERE id(curJob)=$jobid\n" +
            "MATCH (curJob)-[r:JobInvokeSym]->(curSumbol:ElfSymInfo)\n" +
            "MATCH (curSumbol:ElfSymInfo)<-[:HAS_SYMINFO]-(curElfImg:ElfImgInfo)\n" +
            "WHERE id(curElfImg)=$elfImgId\n" +
            "RETURN sum(r.counts)")
    Long getELFImgCount(Long jobid, Long elfImgId);


}
