package com.xttechgroup.scaler.analyzerserv.models.POJO;

import lombok.Data;

@Data
public class VisibilityInfo {
    public Long[] visibleThreads = null;
    public Long[] visibleProcesses = null;
}
