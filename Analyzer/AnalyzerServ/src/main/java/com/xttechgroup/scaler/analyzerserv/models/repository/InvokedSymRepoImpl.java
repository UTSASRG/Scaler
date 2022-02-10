package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.POJO.SymCountQueryResult;
import com.xttechgroup.scaler.analyzerserv.models.POJO.SymTimingQueryResult;
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
    public Long getELFImgCount(Long jobid, Long elfImgId, Long[] visibleProcesses, Long[] visibleThreads) {

        String inProcessStr = "AND (r.processId IN $visibleProcesses)\n";
        String inThreadStr = "AND (r.threadId IN $visibleThreads)\n";

        if (visibleProcesses.length == 0) {
            inProcessStr = "";
        }
        if (visibleThreads.length == 0) {
            inThreadStr = "";
        }


        return this.neo4jClient
                .query("MATCH (curJob:Job)\n" +
                        "WHERE id(curJob)=$jobid\n" +
                        "MATCH (curJob)-[r:JobInvokeSym]->(curSumbol:ElfSym)<-[:HAS_EXTSYM]-(curElfImg:ElfImg)\n" +
                        "WHERE id(curElfImg)=$elfImgId\n" +
                        inProcessStr + inThreadStr +
                        "\nRETURN sum(r.counts)")
                .bind(jobid).to("jobid")
                .bind(elfImgId).to("elfImgId")
                .bind(visibleProcesses).to("visibleProcesses")
                .bind(visibleThreads).to("visibleThreads")
                .fetchAs(Long.class)
                .mappedBy((typeSystem, record) -> record.get(0).asLong())
                .first().get();
    }

    @Override
    public Collection<SymCountQueryResult> getELFImgCountSymbols(Long jobid, Long elfImgId, Long threadId, Long skipSymbolNum,
                                                                 Long visibleSymbolLimit, Long[] visibleThreads, Long[] visibleProcesses) {

        String inProcessStr = "AND (r.processId IN $visibleProcesses)\n";
        String inThreadStr = "AND (r.threadId IN $visibleThreads)\n";

        if (visibleProcesses.length == 0) {
            inProcessStr = "";
        }
        if (visibleThreads.length == 0) {
            inThreadStr = "\n";
        }


        StringBuilder sb = new StringBuilder();
        sb.append("MATCH (curJob:Job)\n" +
                "WHERE id(curJob)=$jobid\n" +
                "MATCH (curJob)-[r:JobInvokeSym]->(curSymbol:ElfSym)<-[:HAS_EXTSYM]-(curElfImg:ElfImg)\n" +
                "WHERE id(curElfImg)=$elfImgId\n"
                + inProcessStr + inThreadStr);

        sb.append("RETURN curSymbol.symbolName,SUM(r.counts) AS invocationCounts\n");
        sb.append("ORDER BY invocationCounts DESC\n");

        if(skipSymbolNum!=null){
            sb.append("SKIP $skipSymbolNum\n");
        }
        if(visibleSymbolLimit!=null){
            sb.append("LIMIT $visibleSymbolLimit");
        }


        return this.neo4jClient
                .query(sb.toString())
                .bind(jobid).to("jobid")
                .bind(elfImgId).to("elfImgId")
                .bind(skipSymbolNum).to("skipSymbolNum")
                .bind(visibleSymbolLimit).to("visibleSymbolLimit")
                .bind(visibleProcesses).to("visibleProcesses")
                .bind(visibleThreads).to("visibleThreads")
                .fetchAs(SymCountQueryResult.class)
                .mappedBy((typeSystem, record) -> new SymCountQueryResult(record.get(1).asLong(), record.get(0).asString()))
                .all();

    }

    @Override
    public Long getELFImgTiming(Long jobid, Long elfImgId, Long[] visibleProcesses, Long[] visibleThreads) {

        String inProcessStr = "AND (r.processId IN $visibleProcesses)\n";
        String inThreadStr = "AND (r.threadId IN $visibleThreads)\n";

        if (visibleProcesses.length == 0) {
            inProcessStr = "";
        }
        if (visibleThreads.length == 0) {
            inThreadStr = "\n";
        }

        return this.neo4jClient
                .query("MATCH (curJob:Job)-[:HAS_IMG]->(curImg:ElfImg)\n" +
                        "WHERE id(curJob)=$jobid\n" +
                        "MATCH (curImg)<-[r:ExtSymInvokeImg]-(invokedSym:ElfSym)\n" +
                        "WHERE id(curImg)=$elfImgId\n" +
                        inProcessStr + inThreadStr +
                        "return sum(r.duration)")
                .bind(jobid).to("jobid")
                .bind(elfImgId).to("elfImgId")
                .bind(visibleProcesses).to("visibleProcesses")
                .bind(visibleThreads).to("visibleThreads")
                .fetchAs(Long.class)
                .mappedBy((typeSystem, record) -> record.get(0).asLong())
                .first().get();
    }

    @Override
    public Collection<SymTimingQueryResult> getELFImgTimingSymbols(Long jobid, Long elfImgId, Long threadId, Long skipSymbolNum,
                                                                   Long visibleSymbolLimit, Long[] visibleThreads, Long[] visibleProcesses) {
        String inProcessStr = "AND (r.processId IN $visibleProcesses)\n";
        String inThreadStr = "AND (r.threadId IN $visibleThreads)\n";

        if (visibleProcesses.length == 0) {
            inProcessStr = "";
        }
        if (visibleThreads.length == 0) {
            inThreadStr = "\n";
        }

        StringBuilder sb = new StringBuilder();
        sb.append("MATCH (curJob:Job)-[:HAS_IMG]->(curImg:ElfImg)\n" +
                "WHERE id(curJob)=$jobid\n" +
                "MATCH (curImg)<-[r:ExtSymInvokeImg]-(invokedSym:ElfSym)\n" +
                "WHERE id(curImg)=$elfImgId\n"
                + inProcessStr + inThreadStr);

        sb.append("RETURN invokedSym.symbolName AS symbolName,SUM(r.duration) AS durations\n");
        sb.append("ORDER BY durations DESC\n");
        if(skipSymbolNum!=null){
            sb.append("SKIP $skipSymbolNum\n");
        }
        if(visibleSymbolLimit!=null){
            sb.append("LIMIT $visibleSymbolLimit\n");
        }


        return this.neo4jClient
                .query(sb.toString())
                .bind(jobid).to("jobid")
                .bind(elfImgId).to("elfImgId")
                .bind(skipSymbolNum).to("skipSymbolNum")
                .bind(visibleSymbolLimit).to("visibleSymbolLimit")
                .bind(visibleProcesses).to("visibleProcesses")
                .bind(visibleThreads).to("visibleThreads")
                .fetchAs(SymTimingQueryResult.class)
                .mappedBy((typeSystem, record) -> new SymTimingQueryResult(record.get(1).asLong(), record.get(0).asString()))
                .all();

    }
}
