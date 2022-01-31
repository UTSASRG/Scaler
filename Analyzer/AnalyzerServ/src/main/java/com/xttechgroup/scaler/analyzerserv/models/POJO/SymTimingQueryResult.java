package com.xttechgroup.scaler.analyzerserv.models.POJO;

import lombok.Data;

@Data
public class SymTimingQueryResult {
    public Long durations;
    public String symbolName;

    public SymTimingQueryResult(Long durations, String symbolName) {
        this.durations = durations;
        this.symbolName = symbolName;
    }
}
