package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.POJO.SymCountQueryResult;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.neo4j.core.Neo4jClient;
import org.springframework.stereotype.Component;

import java.util.Collection;

//https://docs.spring.io/spring-data/neo4j/docs/6.0.5/reference/html/#
@Component
public class InvokedSymRepoImpl implements InvokedSymRepo {
    @Autowired
    Neo4jClient neo4jClient;

    @Override
    public Long getELFImgCount(Long jobid, Long elfImgId) {
        return this.neo4jClient
                .query("MATCH (curJob:Job)\n" +
                        "WHERE id(curJob)=$jobid\n" +
                        "MATCH (curJob)-[r:JobInvokeSym]->(curSumbol:ElfSym)<-[:HAS_EXTSYM]-(curElfImg:ElfImg)\n" +
                        "WHERE id(curElfImg)=$elfImgId\n" +
                        "RETURN sum(r.counts)")
                .bind(jobid).to("jobid")
                .bind(elfImgId).to("elfImgId")
                .fetchAs(Long.class)
                .mappedBy((typeSystem, record) -> record.get(0).asLong())
                .first().get();
    }

    @Override
    public Collection<SymCountQueryResult> getELFImgCountSymbols(Long jobid, Long elfImgId, Long skipSymbolNum,
                                                                 Long visibleSymbolLimit) {
        StringBuilder sb = new StringBuilder();
        sb.append("MATCH (curJob:Job)\n" +
                "WHERE id(curJob)=$jobid\n" +
                "MATCH (curJob)-[r:JobInvokeSym]->(curSymbol:ElfSym)<-[:HAS_EXTSYM]-(curElfImg:ElfImg)\n" +
                "WHERE id(curElfImg)=$elfImgId\n");

        sb.append("RETURN curSymbol.symbolName,r.counts\n");

        if (skipSymbolNum != null) {
            sb.append("SKIP $skipSymbolNum\n");
        }
        if (visibleSymbolLimit != null) {
            sb.append("LIMIT $visibleSymbolLimit\n");
        }
        return this.neo4jClient
                .query(sb.toString())
                .bind(jobid).to("jobid")
                .bind(elfImgId).to("elfImgId")
                .bind(skipSymbolNum).to("skipSymbolNum")
                .bind(visibleSymbolLimit).to("visibleSymbolLimit")
                .fetchAs(SymCountQueryResult.class)
                .mappedBy((typeSystem, record) -> new SymCountQueryResult(record.get(1).asLong(), record.get(0).asString()))
                .all();

    }

//    @Override
//    public Long getELFImgTiming(Long jobid, Long elfImgId) {
//        return this.neo4jClient
//                .query("MATCH (curJob:Job)\n" +
//                        "WHERE id(curJob)=$jobid\n" +
//                        "MATCH (curJob)-[r:JobInvokeSym]->(curSumbol:ElfSym)<-[:HAS_EXTSYM]-(curElfImg:ElfImg)\n" +
//                        "WHERE id(curElfImg)=$elfImgId\n" +
//                        "RETURN sum(r.counts)")
//                .bind(jobid).to("jobid")
//                .bind(elfImgId).to("elfImgId")
//                .fetchAs(Long.class)
//                .mappedBy((typeSystem, record) -> record.get(0).asLong())
//                .first().get();
//    }
//
//    @Override
//    public Collection<SymCountQueryResult> getELFImgTimingSymbols(Long jobid, Long elfImgId, Long skipSymbolNum,
//                                                                 Long visibleSymbolLimit) {
//        StringBuilder sb = new StringBuilder();
//        sb.append("MATCH (curJob:Job)\n" +
//                "WHERE id(curJob)=$jobid\n" +
//                "MATCH (curJob)-[r:JobInvokeSym]->(curSymbol:ElfSym)<-[:HAS_EXTSYM]-(curElfImg:ElfImg)\n" +
//                "WHERE id(curElfImg)=$elfImgId\n");
//
//        sb.append("RETURN curSymbol.symbolName,r.counts\n");
//
//        if (skipSymbolNum != null) {
//            sb.append("SKIP $skipSymbolNum\n");
//        }
//        if (visibleSymbolLimit != null) {
//            sb.append("LIMIT $visibleSymbolLimit\n");
//        }
//        return this.neo4jClient
//                .query(sb.toString())
//                .bind(jobid).to("jobid")
//                .bind(elfImgId).to("elfImgId")
//                .bind(skipSymbolNum).to("skipSymbolNum")
//                .bind(visibleSymbolLimit).to("visibleSymbolLimit")
//                .fetchAs(SymCountQueryResult.class)
//                .mappedBy((typeSystem, record) -> new SymCountQueryResult(record.get(1).asLong(), record.get(0).asString()))
//                .all();
//
//    }
}