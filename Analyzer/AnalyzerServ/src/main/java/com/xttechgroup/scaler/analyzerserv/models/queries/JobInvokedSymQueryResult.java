package com.xttechgroup.scaler.analyzerserv.models.queries;

import com.xttechgroup.scaler.analyzerserv.models.nodes.ElfImgEntity;
import lombok.Data;

@Data
public class JobInvokedSymQueryResult {
    public Long counts;
    public Long threadID;
    public String symName;

    public JobInvokedSymQueryResult(Long counts, String symName) {
        this.counts = counts;
        this.symName = symName;
    }
}
