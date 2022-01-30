package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.POJO.SymCountQueryResult;
import com.xttechgroup.scaler.analyzerserv.models.relations.JobInvokeSym;
import org.springframework.data.neo4j.repository.Neo4jRepository;
import org.springframework.data.neo4j.repository.query.Query;
import org.springframework.transaction.annotation.Transactional;

import java.util.Collection;
import java.util.List;

public interface JobInvokedSymRepo {


    @Transactional(readOnly = true)
    Long getELFImgCount(Long jobid, Long elfImgId);


    @Transactional(readOnly = true)
    Collection<SymCountQueryResult> getELFImgCountSymbols(Long jobid, Long elfImgId);

}
