package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.POJO.LibraryViewQueryResult;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.neo4j.core.Neo4jClient;
import org.springframework.stereotype.Component;

import java.util.Collection;

@Component
public class ProfilingInfoRepoImpl implements ProfilingInfoRepo {
    @Autowired
    Neo4jClient neo4jClient;

    @Override
    public Collection<Long> getThreadIds(Long jobid) {
        return this.neo4jClient
                .query("MATCH (curJob:Job)-[jobSym:JobInvokeSym]->(curSym:ElfSym)\n" +
                        "WHERE id(curJob)=$jobid\n" +
                        "RETURN DISTINCT jobSym.threadId")
                .bind(jobid).to("jobid")
                .fetchAs(Long.class)
                .mappedBy((typeSystem, record) -> record.get(0).asLong())
                .all();
    }

    @Override
    public Collection<Long> getProcessIds(Long jobid) {
        return this.neo4jClient
                .query("MATCH (curJob:Job)-[jobSym:JobInvokeSym]->(curSym:ElfSym)\n" +
                        "WHERE id(curJob)=$jobid\n" +
                        "RETURN DISTINCT jobSym.processId")
                .bind(jobid).to("jobid")
                .fetchAs(Long.class)
                .mappedBy((typeSystem, record) -> record.get(0).asLong())
                .all();
    }

    @Override
    public Long getThreadTotalTime(Long jobid, Long[] processIds, Long[] threadIds) {

        String inProcessStr = "AND (curThread.processId IN $visibleProcesses)\n";
        String inThreadStr = "AND (curThread.threadId IN $visibleThreads)\n";

        if (processIds.length == 0) {
            inProcessStr = "";
        }
        if (threadIds.length == 0) {
            inThreadStr = "\n";
        }

        return this.neo4jClient
                .query("MATCH (curJob:Job)-[:HAS_THREAD]->(curThread:Thread)\n" +
                        "WHERE id(curJob)=$jobid\n" +
                        inProcessStr + inThreadStr +
                        "RETURN SUM(curThread.totalTime)")
                .bind(jobid).to("jobid")
                .bind(processIds).to("visibleProcesses")
                .bind(threadIds).to("visibleThreads")
                .fetchAs(Long.class)
                .mappedBy((typeSystem, record) -> record.get(0).asLong())
                .first().get();
    }

    @Override
    public Collection<LibraryViewQueryResult> getLibraryCenteredData(Long jobid, Long elfImgId, Long[] processIds, Long[] threadIds) {
        String inProcessStr = "AND (r.processId IN $visibleProcesses)\n";
        String inThreadStr = "AND (r.threadId IN $visibleThreads)\n";

        if (processIds.length == 0) {
            inProcessStr = "";
        }
        if (threadIds.length == 0) {
            inThreadStr = "\n";
        }
        return this.neo4jClient
                .query("MATCH (curJob:Job)-[:HAS_IMG]->(curImg:ElfImg)\n" +
                        "WHERE id(curJob)=$jobid AND id(curImg)=$elfImgId\n" +
                        "MATCH (curImg)<-[r:ExtSymInvokeImg]-(invokedSym:ElfSym)\n" +
                        "WHERE true\n" +
                        inProcessStr + inThreadStr +
                        "MATCH (invokedSym)<-[:HAS_EXTSYM]-(calleeImg:ElfImg)\n" +
                        "RETURN calleeImg.filePath,SUM(r.duration)")
                .bind(jobid).to("jobid")
                .bind(elfImgId).to("elfImgId")
                .bind(processIds).to("visibleProcesses")
                .bind(threadIds).to("visibleThreads")
                .fetchAs(LibraryViewQueryResult.class)
                .mappedBy((typeSystem, record) -> new LibraryViewQueryResult(record.get(0).asString(), record.get(1).asLong()))
                .all();
    }
}
