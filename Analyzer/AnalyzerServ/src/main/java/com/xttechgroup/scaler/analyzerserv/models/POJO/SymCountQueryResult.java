package com.xttechgroup.scaler.analyzerserv.models.POJO;

import lombok.Data;
import org.springframework.data.neo4j.core.schema.Id;

@Data
public class SymCountQueryResult {
    public Long counts;
    public String symbolName;

    public SymCountQueryResult(Long counts, String symbolName) {
        this.counts = counts;
        this.symbolName = symbolName;
    }
}
