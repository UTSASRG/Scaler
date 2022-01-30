package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.POJO.SymCountQueryResult;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.neo4j.core.Neo4jClient;
import org.springframework.stereotype.Component;

import java.util.Collection;
import java.util.List;

//https://docs.spring.io/spring-data/neo4j/docs/6.0.5/reference/html/#
@Component
public class JobInvokedSymRepoImpl implements JobInvokedSymRepo {
    @Autowired
    Neo4jClient neo4jClient;

    @Override
    public Long getELFImgCount(Long jobid, Long elfImgId) {
        return this.neo4jClient
                .query("MATCH (curJob:Job)\n" +
                        "WHERE id(curJob)=$jobid\n" +
                        "MATCH (curJob)-[r:JobInvokeSym]->(curSumbol:ElfSymInfo)<-[:HAS_SYMINFO]-(curElfImg:ElfImgInfo)\n" +
                        "WHERE id(curElfImg)=$elfImgId\n" +
                        "RETURN sum(r.counts)")
                .bind(jobid).to("jobid")
                .bind(elfImgId).to("elfImgId")
                .fetchAs(Long.class)
                .mappedBy((typeSystem, record) -> record.get(0).asLong())
                .first().get();
    }

    @Override
    public Collection<SymCountQueryResult> getELFImgCountSymbols(Long jobid, Long elfImgId) {
        return this.neo4jClient
                .query("MATCH (curJob:Job)\n" +
                        "WHERE id(curJob)=$jobid\n" +
                        "MATCH (curJob)-[r:JobInvokeSym]->(curSymbol:ElfSymInfo)<-[:HAS_SYMINFO]-(curElfImg:ElfImgInfo)\n" +
                        "WHERE id(curElfImg)=$elfImgId\n" +
                        "RETURN curSymbol.symbolName,r.counts")
                .bind(jobid).to("jobid")
                .bind(elfImgId).to("elfImgId")
                .fetchAs(SymCountQueryResult.class)
                .mappedBy((typeSystem, record) -> new SymCountQueryResult(record.get(1).asLong(), record.get(0).asString()))
                .all();
    }
}
