package com.xttechgroup.scaler.analyzerserv.models.POJO;

import lombok.Data;

@Data
public class SymTimingQueryResult {
    public Long cycles;
    public String symbolName;

    public SymTimingQueryResult(Long cycles, String symbolName) {
        this.cycles = cycles;
        this.symbolName = symbolName;
    }
}
