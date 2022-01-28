package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.nodes.ELFSymEntity;
import com.xttechgroup.scaler.analyzerserv.models.nodes.JobEntity;
import com.xttechgroup.scaler.analyzerserv.models.queries.JobInvokedSymQueryResult;
import org.springframework.data.neo4j.repository.Neo4jRepository;
import org.springframework.data.neo4j.repository.query.Query;

import java.util.List;

public interface JobInvokedSymRepo extends Neo4jRepository<JobEntity, Long> {


    @Query("MATCH (curJob:Job)-[r:JobInvokeSym]->(node1:ElfSymInfo)\n" +
            "WHERE id(curJob)=37453 AND r.counts<>0\n" +
            "return r.counts AS counts,curSym.symbolName AS symName")
    List<JobInvokedSymQueryResult> getJobInvokedSymbols(Long jobid);

}
