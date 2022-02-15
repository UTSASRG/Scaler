package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.POJO.LibraryViewQueryResult;
import org.springframework.transaction.annotation.Transactional;

import java.util.Collection;

public interface ProfilingInfoRepo {

    @Transactional(readOnly = true)
    Collection<Long> getThreadIds(Long jobid);

    @Transactional(readOnly = true)
    Collection<Long> getProcessIds(Long jobid);

    @Transactional(readOnly = true)
    Long getThreadTotalTime(Long jobid, Long[] processId, Long[] threadId);

    @Transactional(readOnly = true)
    Collection<LibraryViewQueryResult> getLibraryCenteredData(Long jobid, Long elfImgId, Long[] processIds, Long[] threadIds);

}
