package com.xttechgroup.scaler.analyzerserv.models.POJO;

import lombok.Data;

import java.util.List;

@Data
public class MultipleElfIds {
    public Long jobid = null;
    public Long[] elfImgIds = null;
    public Long[] visibleThreads = null;
    public Long[] visibleProcesses = null;
}
