package com.xttechgroup.scaler.analyzerserv.models.repository;

import com.xttechgroup.scaler.analyzerserv.models.nodes.JobEntity;

public interface JobRepo {
    Long newJob();

    JobEntity findById(Long jobID);

}
