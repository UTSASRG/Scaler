package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.POJO.SymCountQueryResult;
import com.xttechgroup.scaler.analyzerserv.models.POJO.SymTimingQueryResult;
import org.springframework.transaction.annotation.Transactional;

import java.util.Collection;

public interface InvokedSymRepo {


    @Transactional(readOnly = true)
    Long getELFImgCount(Long jobid, Long elfImgId, Long[] visibleProcesses, Long[] visibleThreads);


    @Transactional(readOnly = true)
    Collection<SymCountQueryResult> getELFImgCountSymbols(Long jobid, Long elfImgId, Long threadId, Long skipSymbol,
                                                          Long visibleSymbolLimit, Long[] visibleThreads, Long[] visibleProcesses);

    @Transactional(readOnly = true)
    public Long getELFImgTiming(Long jobid, Long elfImgId, Long[] visibleProcesses, Long[] visibleThreads);

    @Transactional(readOnly = true)
    Collection<SymTimingQueryResult> getELFImgTimingSymbols(Long jobid, Long elfImgId, Long threadId, Long skipSymbol,
                                                            Long visibleSymbolLimit, Long[] visibleThreads, Long[] visibleProcesses);
}
